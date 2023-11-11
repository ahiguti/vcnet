
#include <vcnet_usb.h>

#define VCNET_SERIAL Serial
  // ログ出力先

#if defined(ARDUINO_ARCH_AVR)
  // Board: SparkFun AVR Boards - SparkFun Pro Micro
  // Board: Adafruit Boards - Adafruit ItsyBitsy 32u4 3V 8MHz
  #define USE_USB_MOD
  #define USE_USB
  #define USE_KEYBOARD
  #define USE_MOUSE
  #define USE_ABSMOUSE
  #define USE_JOYSTICK
#elif defined(ARDUINO_ARCH_SAMD)
  // Board: Adafruit SAMD(32bits ARM Cortex-M0+) Boards
  //          - Adafruit ItsyBitsy M0 Express (SAMD21)
  //          - Adafruit QT Py M0 (SAMD21)
  #define USE_USB_MOD
  #define USE_USB
  #define USE_KEYBOARD
  #define USE_MOUSE
  #undef  USE_ABSMOUSE
  #define USE_JOYSTICK
#elif defined (ARDUINO_ARCH_RP2040)
  #ifdef LWIP_IPV6
    // Board: Raspberry Pi RP2040 - Raspberry Pi Pico
    // 注意: ボードファイルにパッチを当てる必要がある。
    // rp2040_3_1_0_usb_fix.patch
    #undef  USE_USB_MOD
    #define USE_USB
    #define USE_KEYBOARD
    #define USE_MOUSE
    #undef  USE_ABSMOUSE
    #define USE_JOYSTICK
    #define SEND_REPORT_MAX_RETRY 10000
    #define HAS_BOOTSEL_BUTTON
  #else
    // Board: Arduino Mbed OS RP2040 Boards - Raspberry Pi Pico
    // USBのreportが届かないことがある問題が未解決。
    #define USE_USB_MOD
    #define USE_USB
    #define USE_KEYBOARD
    #define USE_MOUSE
    #undef USE_ABSMOUSE
    #define USE_JOYSTICK
  #endif
#elif defined(ARDUINO_ARCH_ESP32)
  // Board: ESP32 Arduino - ESP32 Dev Module
  #define USE_BLE
  #define USE_KEYBOARD
  #define USE_MOUSE
  #undef  USE_ABSMOUSE
  #undef  USE_JOYSTICK
#else
  // その他
  // #define USE_USB_MOD
  #define USE_USB
  #define USE_KEYBOARD
  #define USE_MOUSE
  // #define USE_ABSMOUSE
  // #define USE_JOYSTICK
  // #define USB_DELAY_USEC 3000
  // #undef VCNET_SERIAL
  // #define VCNET_SERIAL Serial1
#endif

#ifdef USE_USB_MOD
#ifdef USE_MOUSE
#include "MouseMod.h"
MouseMod_ MouseMod;
#endif
#ifdef USE_KEYBOARD
#include "KeyboardMod.h"
KeyboardMod_ KeyboardMod;
#endif
#ifdef USE_ABSMOUSE
#include "AbsMouseMod.h"
AbsMouseMod_ AbsMouseMod;
#endif
#ifdef USE_JOYSTICK
#include "JoystickMod.h"
JoystickMod_ JoystickMod(4);
#endif
#endif

#if defined(USE_USB) && !defined(USE_USB_MOD)
#ifdef USE_MOUSE
#include "Mouse.h"
#endif
#ifdef USE_KEYBOARD
#include "Keyboard.h"
#endif
#ifdef USE_JOYSTICK
#include "Joystick.h"
#endif
#endif

#ifdef USE_BLE
#include "BleCombo.h"
#endif

namespace {

int8_t mcu_id = -1;

enum verbose_e {
  verbose_e_spi = 1,
  verbose_e_mouse = 2,
  verbose_e_mouse_button = 3,
  verbose_e_keyboard = 4,
  verbose_e_joystick = 5,
}; // シリアルポートに書くとハングすることがあるので注意。
uint8_t verbose = 0;

enum { REG_WORDS = 16 };
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

uint8_t mouse_seq;

void
vcnet_usb_setup(int8_t mcu_id0, uint8_t verbose0)
{
  mcu_id = mcu_id0;
  verbose = verbose0;
  #ifdef USE_USB_MOD
    // UnoR4はbegin()の定義が漏れている
    #if !defined(ARDUINO_UNOR4_MINIMA) && !defined(ARDUINO_UNOR4_WIFI)
    DynamicHID().begin();
    #endif
    #ifdef USE_MOUSE
    MouseMod.begin();
    #endif
    #ifdef USE_ABSMOUSE
    AbsMouseMod.init(32767, 32767, false);
    #endif
    #ifdef USE_KEYBOARD
    KeyboardMod.begin();
    #endif
    #ifdef USE_JOYSTICK
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
    #endif
  #endif // USE_USB_MOD
  #if defined(USE_USB) && !defined(USE_USB_MOD)
    #ifdef USE_MOUSE
    Mouse.begin();
    // Mouse.useManualSend(true); // FIXME?
    #endif
    #ifdef USE_KEYBOARD
    Keyboard.begin();
    #endif
    #ifdef USE_JOYSTICK
    Joystick.begin();
    Joystick.useManualSend(true);
    #endif
  #endif
  #ifdef USE_BLE
    #ifdef USE_KEYBOARD
    Keyboard.begin();
    #endif
    #ifdef USE_MOUSE
    Mouse.begin();
    #endif
  #endif
  #ifdef DOTSTAR_DATAPIN
  dotstar.begin();
  dotstar.setBrightness(80);
  dotstar.show();
  #endif
}

void dump_reg_values(uint16_t const *reg_values)
{
  for (uint8_t i = 0; i < REG_WORDS; ++i) {
    VCNET_SERIAL.print(" ");
    VCNET_SERIAL.print(reg_values[i], HEX);
  }
  VCNET_SERIAL.println("");
}

void vcnet_usb_update_regarray(uint16_t const *reg_values, uint16_t nelems)
{
  if (nelems < REG_WORDS) {
    return;
  }
  if (mcu_id >= 0 && (reg_values[0] & 0x7f) != mcu_id) {
    // 制御対象が自分ではない
    if (verbose > 0) {
      VCNET_SERIAL.print("skip mcu_id ");
      VCNET_SERIAL.println(reg_values[0] & 0xf7);
    }
    return;
  }
  #if 0
  dump_reg_values(reg_values);
  #endif
  // mouse buttons
  bool send_mouse = false;
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
      #ifdef USE_USB
      #ifdef USE_MOUSE
      #ifdef USE_USB_MOD
      MouseMod.buttons(mb0);
      #else
      Mouse.release(~mb0);
      Mouse.press(mb0);
      send_mouse = true;
      #endif
      #endif
      #ifdef USE_ABSMOUSE
      AbsMouseMod.buttons(mb1);
      AbsMouseMod.report();
      #endif
      #endif // USE_USB
      #ifdef USE_BLE
      Mouse.release(~mb0);
      Mouse.press(mb0);
      #endif
      if (verbose == verbose_e_mouse_button) {
        VCNET_SERIAL.print("mousebutton ");
        VCNET_SERIAL.println(mb, HEX);
      }
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
      #ifdef USE_BLE
      Mouse.move(x, y, wy, wx);
      #endif
      #ifdef USE_USB
      #ifdef USE_MOUSE
      #ifdef USE_USB_MOD
      MouseMod.move(x, y, wy, wx);
      #else
      Mouse.move(x, y, wy /*, wx */); // FIXME?
      send_mouse = true;
      #endif
      #endif
      #endif
      if (verbose == verbose_e_mouse) {
        VCNET_SERIAL.print("mouse ");
        VCNET_SERIAL.print(x);
        VCNET_SERIAL.print(" ");
        VCNET_SERIAL.print(y);
        VCNET_SERIAL.print(" ");
        VCNET_SERIAL.print(wy);
        VCNET_SERIAL.print(" ");
        VCNET_SERIAL.println(wx);
      }
    }
    mouse_seq = seq;
  }
  // absmouse
  {
    if (reg_values[3] != reg_values_prev[3] ||
      reg_values[4] != reg_values_prev[4]) {
      #ifdef USE_USB_MOD
      #ifdef USE_ABSMOUSE
      AbsMouseMod.move(reg_values[3], reg_values[4]);
      AbsMouseMod.report();
      #endif
      #endif
    }
  }
  #if defined(USE_USB) && !defined(USE_USB_MOD) && defined(USE_MOUSE)
  if (send_mouse) {
    unsigned long const tk0 = micros();
    int i = 0;
    #ifndef ARDUINO_UNOR4_MINIMA // FIXME
    for (; i < SEND_REPORT_MAX_RETRY; ++i) {
      if (!Mouse.send_now()) {
      } else {
        break;
      }
    }
    #endif
    if (verbose == verbose_e_mouse) {
      if (i != 0) {
        unsigned long const tk1 = micros();
        VCNET_SERIAL.print("mouse report ");
        VCNET_SERIAL.print(i);
        VCNET_SERIAL.print(" time ");
        VCNET_SERIAL.println(tk1 - tk0);
      }
    }
  }
  #endif
  // keyboard
  {
    if (reg_values[6] != reg_values_prev[6] ||
      reg_values[7] != reg_values_prev[7] ||
      reg_values[8] != reg_values_prev[8] ||
      (reg_values[5] >> 8) != (reg_values_prev[5] >> 8)) {
      #ifdef USE_KEYBOARD
      #ifdef USE_USB_MOD
      uint8_t keys[6];
      keys[0] = reg_values[6];
      keys[1] = reg_values[6] >> 8;
      keys[2] = reg_values[7];
      keys[3] = reg_values[7] >> 8;
      keys[4] = reg_values[8];
      keys[5] = reg_values[8] >> 8;
      uint8_t kmod = reg_values[5] >> 8;
      KeyboardMod.sendReport(keys, kmod);
      #elif defined(USE_USB)
      KeyReport keyreport;
      keyreport.modifiers = reg_values[5] >> 8;
      keyreport.keys[0] = reg_values[6];
      keyreport.keys[1] = reg_values[6] >> 8;
      keyreport.keys[2] = reg_values[7];
      keyreport.keys[3] = reg_values[7] >> 8;
      keyreport.keys[4] = reg_values[8];
      keyreport.keys[5] = reg_values[8] >> 8;
      unsigned long const tk0 = micros();
      int i = 0;
      #ifndef ARDUINO_UNOR4_MINIMA // FIXME
      for (; i < SEND_REPORT_MAX_RETRY; ++i) {
        if (!Keyboard.sendReport(&keyreport)) {
        } else {
          break;
        }
      }
      #endif
      if (verbose == verbose_e_keyboard) {
        if (i != 0) {
          unsigned long const tk1 = micros();
          VCNET_SERIAL.print("keyboard report ");
          VCNET_SERIAL.print(i);
          VCNET_SERIAL.print(" time ");
          VCNET_SERIAL.println(tk1 - tk0);
        }
      }
      #elif defined(USE_BLE)
      KeyReport keyreport;
      keyreport.modifiers = reg_values[5] >> 8;
      keyreport.keys[0] = reg_values[6];
      keyreport.keys[1] = reg_values[6] >> 8;
      keyreport.keys[2] = reg_values[7];
      keyreport.keys[3] = reg_values[7] >> 8;
      keyreport.keys[4] = reg_values[8];
      keyreport.keys[5] = reg_values[8] >> 8;
      Keyboard.sendReport(&keyreport);
      #endif
      #endif // USE_KEYBOARD
      #if 0
      if (verbose == verbose_e_keyboard) {
        VCNET_SERIAL.print("key ");
        VCNET_SERIAL.print(keyreport.keys[0], HEX);
        VCNET_SERIAL.print(" ");
        VCNET_SERIAL.print(keyreport.keys[1], HEX);
        VCNET_SERIAL.print(" ");
        VCNET_SERIAL.print(keyreport.keys[2], HEX);
        VCNET_SERIAL.print(" ");
        VCNET_SERIAL.print(keyreport.keys[3], HEX);
        VCNET_SERIAL.print(" ");
        VCNET_SERIAL.print(keyreport.keys[4], HEX);
        VCNET_SERIAL.print(" ");
        VCNET_SERIAL.print(keyreport.keys[5], HEX);
        VCNET_SERIAL.print(" ");
        VCNET_SERIAL.println(keyreport.modifiers, HEX);
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
        #ifdef USE_JOYSTICK
        #ifdef USE_USB_MOD
        JoystickMod.setButton(i, v & 0x01);
        #else
        Joystick.setButton(i, v & 0x01);
        #endif
        #endif
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
      #ifdef USE_JOYSTICK
      #ifdef USE_USB_MOD
      JoystickMod.setHatSwitch(0, hv);
      #else
      Joystick.hat(0, hv);
      #endif
      #endif
      if (verbose == verbose_e_joystick) {
        VCNET_SERIAL.print("hat,buttons ");
        VCNET_SERIAL.print(reg_values[9], HEX);
        VCNET_SERIAL.print(" prev ");
        VCNET_SERIAL.println(reg_values_prev[9], HEX);
      }
    }
    // analog sticks
    for (uint8_t i = 0; i < 6; ++i) {
      if (reg_values[10 + i] != reg_values_prev[10 + i]) {
        #ifdef USE_JOYSTICK
        send_report = true;
        #ifdef USE_USB_MOD
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
        #else
        uint16_t v = reg_values[10 + i] + 0x8000u;
        v >>= 6;
        switch (i) {
        case 0: Joystick.X(v); break;
        case 1: Joystick.Y(v); break;
        case 2: Joystick.sliderLeft(v); break;
        case 3: Joystick.sliderRight(v); break;
        case 4: Joystick.Z(v); break;
        case 5: Joystick.Zrotate(v); break;
        default: break;
        }
        #endif
        #endif
      }
    }
    if (send_report) {
      #ifdef USE_JOYSTICK
      #ifdef USE_USB_MOD
      JoystickMod.sendState();
      #else
      unsigned long const tj0 = micros();
      int i = 0;
      for (; i < SEND_REPORT_MAX_RETRY; ++i) {
        if (!Joystick.send_now()) {
        } else {
          break;
        }
      }
      if (verbose == verbose_e_joystick) {
        if (i != 0) {
          unsigned long const tj1 = micros();
          VCNET_SERIAL.print("joystick report ");
          VCNET_SERIAL.print(i);
          VCNET_SERIAL.print(" time ");
          VCNET_SERIAL.println(tj1 - tj0);
        }
      }
      #endif
      #endif
    }
  }
  if (verbose == verbose_e_spi) {
    bool nodiff = true;
    for (uint8_t i = 0; i < REG_WORDS; ++i) {
      if (reg_values_prev[i] != reg_values[i]) {
        nodiff = false;
      }
    }
    if (!nodiff) {
      dump_reg_values(reg_values);
    }
  }
  // reg_values_prevへコピーする
  for (uint8_t i = 0; i < REG_WORDS; ++i) {
    reg_values_prev[i] = reg_values[i];
  }
  #if defined USB_DELAY_USEC
  delayMicroseconds(USB_DELAY_USEC);
  #endif
}

}; // anonymous namespace

void
vcnet_usb::begin(int8_t mcu_id0, uint8_t verbose0)
{
  vcnet_usb_setup(mcu_id0, verbose0);
}

void
vcnet_usb::update_regarray(uint16_t const *reg_values, uint16_t nelems)
{
  vcnet_usb_update_regarray(reg_values, nelems);
}

