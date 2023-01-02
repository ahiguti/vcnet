
// vcnetのUSB HID操作用MCUの処理。SPIでFPGAからデータをコピーし、その内容をUSB HIDに反映する。
// sparkfun pro microとraspberry pi picoで動作。

#include <SPI.h>
#include "MouseMod.h"
#include "KeyboardMod.h"
#include "AbsMouseMod.h"
#include "JoystickMod.h"

MouseMod_ MouseMod;
AbsMouseMod_ AbsMouseMod;
KeyboardMod_ KeyboardMod;
JoystickMod_ JoystickMod(4);

uint8_t verbose = 0; // シリアルポートに書くとハングすることがあるので注意。

void setup() {
  Serial.begin(115200);
  //  while (!Serial) { }
  Serial.println("vcnet spi");
  SPI.begin();
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    // SPIトランザクションを実行しっぱなしにする。FPGA側は
    // SCLKが一定時間変化しなかったらトランザクション終了したと
    // 判断するようにしている。
    // pi picoはLSBFIRSTをサポートしていないようなのでMSBFIRST。
  DynamicHID().begin();
  MouseMod.begin();
  AbsMouseMod.init(32767, 32767, false);
  KeyboardMod.begin();
  JoystickMod.begin(false);
  JoystickMod.setXAxisRange(-32767, 32767);
  JoystickMod.setYAxisRange(-32767, 32767);
  JoystickMod.setZAxisRange(-32767, 32767);
  JoystickMod.setRxAxisRange(-32767, 32767);
  JoystickMod.setRyAxisRange(-32767, 32767);
  JoystickMod.setRzAxisRange(-32767, 32767);
  JoystickMod.setRudderRange(-32767, 32767);
  JoystickMod.setThrottleRange(-32767, 32767);
  JoystickMod.setAcceleratorRange(-32767, 32767);
  JoystickMod.setBrakeRange(-32767, 32767);
  JoystickMod.setSteeringRange(-32767, 32767);
}

#define REG_WORDS 32
  // 16bit単位のワード数

unsigned long spi_time_us = 0;

uint8_t mouse_seq;
uint16_t reg_values[REG_WORDS + 2];
  // SPIで取得したFPGA上のレジスタ配列のコピー。末尾4byteはmagic number。
uint16_t reg_values_prev[REG_WORDS];
  // reg_valuesの以前の値
// +0  (1) mcu id (最上位bitは使用しない)
// +1  (1) mouse seq (mouse moveの値が更新されたときincrement)
// +2  (1) mouse delta x
// +3  (1) mouse delta y
// +4  (1) mouse wheel x
// +5  (1) mouse wheel y
// +6  (2) absmouse x
// +8  (2) absmouse y
// +10 (1) mouse/absmouse button(最上位bitが1のときabsmousebutton)
// +11 (1) keymod
// +12 (6) keycode
// +18 (1) gc button
// +19 (1) gc button 4bit, hat 4bit
// +20 (12) analog axes
// +32 (-) magic number

void dump_reg_values()
{
  for (uint8_t i = 0; i < REG_WORDS + 2; ++i) {
    Serial.print(" ");
    Serial.print(reg_values[i], HEX);
  }
  Serial.print(" time=");
  Serial.println(spi_time_us);
}

void loop() {
  // SPIを使ってFPGAのレジスタ配列から値をコピーする。
  {
    unsigned long const t0 = micros();
    #ifndef ARDUINO_ARCH_RP2040
    // sparkfun pro microだと割り込みを禁止しないとデータが化ける
    // pi picoだと割り込みを禁止すると異常終了する
    noInterrupts();
    #endif
    for (uint8_t i = 0; i < REG_WORDS + 2; ++i) {
      uint8_t v0 = SPI.transfer(0);
      uint8_t v1 = SPI.transfer(0);
      reg_values[i] = (((uint16_t)(v1)) << 8) | v0;
    }
    #ifndef ARDUINO_ARCH_RP2040
    interrupts();
    #endif
    unsigned long const t1 = micros();
    spi_time_us = t1 - t0;
    if (spi_time_us > 800 || reg_values[REG_WORDS + 0] != 0xbeef || reg_values[REG_WORDS + 1] != 0xdead) {
      // 通常は250us程度で終了する。時間がかかりすぎているときはFPGA側が
      // オフセットを巻き戻しているので、得られたデータはオフセットがずれている
      // ことがある。そのため破棄する。magic numberが一致しない場合も破棄する。
      if (verbose) {
        Serial.println("spi recv discarded");
        dump_reg_values();
      }
      delay(3);
      return;
    }
  }
  // mouse buttons
  {
    uint8_t mb = reg_values[5] & 0xff;
    if (mb != (reg_values_prev[5] & 0xff)) {
      uint8_t mb0 = 0;
      uint8_t mb1 = 0;
      if ((mb & 0x80) == 0) {
        mb0 = mb & 0x7f;
      } else {
        mb1 = mb & 0x7f;
      }
      MouseMod.buttons(mb0);
      AbsMouseMod.buttons(mb1);
      AbsMouseMod.report();
      #if 0
      Serial.print("mousebutton ");
      Serial.println(mb, HEX);
      #endif
    }
  }
  // mouse move
  {
    uint8_t seq = reg_values[0] >> 8;
    if (seq != mouse_seq || (reg_values[1] != reg_values_prev[1] ||
      reg_values[2] != reg_values_prev[2])) {
      int8_t x = (int8_t)(reg_values[1] & 0xff);
      int8_t y = (int8_t)((reg_values[1] >> 8) & 0xff);
      int8_t wx = (int8_t)(reg_values[2] & 0xff);
      int8_t wy = (int8_t)((reg_values[2] >> 8) & 0xff);
      MouseMod.move(x, y, wy, wx);
      #if 0
      Serial.print("mouse ");
      Serial.print(x);
      Serial.print(" ");
      Serial.print(y);
      Serial.print(" ");
      Serial.print(wy);
      Serial.print(" ");
      Serial.println(wx);
      #endif
    }
    mouse_seq = seq;
  }
  // absmouse
  {
    if (reg_values[3] != reg_values_prev[3] ||
      reg_values[4] != reg_values_prev[4]) {
      AbsMouseMod.move(reg_values[3], reg_values[4]);
      AbsMouseMod.report();
    }
  }
  // keyboard
  {
    if (reg_values[6] != reg_values_prev[6] ||
      reg_values[7] != reg_values_prev[7] ||
      reg_values[8] != reg_values_prev[8] ||
      (reg_values[5] >> 8) != (reg_values_prev[5] >> 8)) {
      uint8_t keys[6];
      keys[0] = reg_values[6];
      keys[1] = reg_values[6] >> 8;
      keys[2] = reg_values[7];
      keys[3] = reg_values[7] >> 8;
      keys[4] = reg_values[8];
      keys[5] = reg_values[8] >> 8;
      uint8_t kmod = reg_values[5] >> 8;
      KeyboardMod.sendReport(keys, kmod);
      #if 0
      if (verbose) {
        Serial.print("key ");
        Serial.print(keys[0], HEX);
        Serial.print(" ");
        Serial.print(keys[1], HEX);
        Serial.print(" ");
        Serial.print(keys[2], HEX);
        Serial.print(" ");
        Serial.print(keys[3], HEX);
        Serial.print(" ");
        Serial.print(keys[4], HEX);
        Serial.print(" ");
        Serial.print(keys[5], HEX);
        Serial.print(" ");
        Serial.println(kmod, HEX);
      }
      #endif
    }
  }
  // game controller
  {
    // buttons
    bool send_report = false;
    if (reg_values[9] != reg_values_prev[9]) {
      send_report = true;
      uint16_t v = reg_values[9];
      for (uint8_t i = 0; i < 12; ++i) {
        JoystickMod.setButton(i, v & 0x01);
        v >>= 1;
      }
      int16_t hv = -1;
      switch (v) {
      case  1: hv = 0; break;
      case  3: hv = 45; break;
      case  2: hv = 90; break;
      case  6: hv = 135; break;
      case  4: hv = 180; break;
      case 12: hv = 225; break;
      case  8: hv = 270; break;
      case  9: hv = 315; break;
      default: hv = -1; break;
      }
      JoystickMod.setHatSwitch(0, hv);
    }
    // analog sticks
    for (uint8_t i = 0; i < 6; ++i) {
      if (reg_values[10 + i] != reg_values_prev[10 + i]) {
        send_report = true;
        int16_t v = (int16_t)reg_values[10 + i];
        switch (i) {
        case 0: JoystickMod.setXAxis(v); break;
        case 1: JoystickMod.setYAxis(v); break;
        case 2: JoystickMod.setRxAxis(v); break;
        case 3: JoystickMod.setRyAxis(v); break;
        case 4: JoystickMod.setZAxis(v); break;
        case 5: JoystickMod.setRzAxis(v); break;
        default: break;
        }
      }
    }
    if (send_report) {
      JoystickMod.sendState();
    }
  }
  if (verbose) {
    bool nodiff = true;
    for (uint8_t i = 0; i < REG_WORDS; ++i) {
      if (reg_values_prev[i] != reg_values[i]) {
        nodiff = false;
      }
    }
    if (!nodiff) {
      dump_reg_values();
    }
  }
  // reg_values_prevへコピーする
  for (uint8_t i = 0; i < REG_WORDS; ++i) {
    reg_values_prev[i] = reg_values[i];
  }
  delay(2);
}
