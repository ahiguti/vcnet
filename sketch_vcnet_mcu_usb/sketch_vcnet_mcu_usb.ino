
#include <vcnet_usb.h>

#define VCNET_MCU_ID -1
  // 制御対象MCUを区別する7bitの値。クライアントの設定項目spiの値。
  // ボードへ書き込むときにこれを変更する。VCNET_USE_BITBANGでない
  // ときはこれを-1にしておく。
#undef VCNET_USE_BITBANG
  // これを定義するとbitbangingでデータを取得、そうでなければSPIでデータを取得。

#ifdef VCNET_USE_BITBANG
  #include <bitbang_receiver.h>
  bitbang_4bit_receiver bb4_receiver;
#else
  #include <SPI.h>
#endif

#ifdef ADAFRUIT_ITSYBITSY_M0
  // ボード上のLEDが眩しいので消灯する
  #define DOTSTAR_DATAPIN     41
  #define DOTSTAR_CLOCKPIN    40
  #define DOTSTAR_COLOR_ORDER DOTSTAR_BGR
#endif

#ifdef DOTSTAR_DATAPIN
  #include <Adafruit_DotStar.h>
  Adafruit_DotStar dotstar(1, DOTSTAR_DATAPIN, DOTSTAR_CLOCKPIN,
    DOTSTAR_COLOR_ORDER);
#endif

enum { REG_WORDS = 16 };
vcnet_usb vcnet_usb_inst;
uint16_t reg_values[REG_WORDS + 2];

void setup() {
  Serial.begin(115200);
  //  while (!Serial) { }
  #ifdef VCNET_USE_BITBANG
    Serial.println("vcnet bb4");
    bb4_receiver.begin();
  #else
    Serial.println("vcnet spi");
    SPI.begin();
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
      // SPIトランザクションを実行しっぱなしにする。FPGA側は
      // SCLKが一定時間変化しなかったらトランザクション終了したと
      // 判断するようにしている。
      // pi picoはLSBFIRSTをサポートしていないようなのでMSBFIRST。
  #endif
  #ifdef DOTSTAR_DATAPIN
    dotstar.begin();
    dotstar.setBrightness(80);
    dotstar.show();
  #endif
  vcnet_usb_inst.begin(VCNET_MCU_ID, 0);
}

void loop() {
  // SPIを使ってFPGAのレジスタ配列から値をコピーする。
  delay(3);
    // fpga側は静止時間を見てトランザクション開始を判断しているので、
    // 必ず一定時間を空けなければならない。
  unsigned long const t0 = micros();
  #ifdef VCNET_USE_BITBANG
  {
    uint16_t n = bb4_receiver.recv((uint8_t *)&reg_values[0],
      sizeof(reg_values));
    if (n != REG_WORDS * sizeof(uint16_t)) {
      return;
    }
  }
  #else
  {
    #if !defined(ARDUINO_ARCH_RP2040) || defined(LWIP_IPV6)
    // sparkfun pro microだと割り込みを禁止しないとデータが化ける
    // mbed pi picoだと割り込みを禁止すると異常終了する
    noInterrupts();
    #endif
    for (uint8_t i = 0; i < REG_WORDS + 2; ++i) {
      uint8_t v0 = SPI.transfer(0);
      uint8_t v1 = SPI.transfer(0);
      reg_values[i] = (((uint16_t)(v1)) << 8) | v0;
    }
    #if !defined(ARDUINO_ARCH_RP2040) || defined(LWIP_IPV6)
    interrupts();
    #endif
    unsigned long const t1 = micros();
    unsigned long const spi_time_us = t1 - t0;
    if (spi_time_us > 2000 || reg_values[REG_WORDS + 0] != 0xbeef ||
      reg_values[REG_WORDS + 1] != 0xdead) {
      // 通常は250us程度で終了する。時間がかかりすぎているときはFPGA側が
      // オフセットを巻き戻しているので、得られたデータはオフセットがずれている
      // ことがある。そのため破棄する。magic numberが一致しない場合も破棄する。
      return;
    }
  }
  #endif
  vcnet_usb_inst.update_regarray(reg_values, REG_WORDS);
}
