
#include <bitbang_sender.h>

#define DBG(x)

#if defined(ARDUINO_UNOR4_MINIMA) || defined(ARDUINO_UNOR4_WIFI)
// UnoR4 Port01: { (A5/SCL), (A4/SDA), D5, D4, D3, D2, D6, D7 }
#define bitbang_last_mask 0x80
#define bitbang_clk_mask 0x40
#define bitbang_data_mask 0x3c
#define bitbang_all_mask 0xfc
#define bitbang_pins { 5, 4, 3, 2, 6, 7 }
#define bitbang_port_write(x) \
  R_IOPORT_PortWrite(nullptr, BSP_IO_PORT_01, (x) & bitbang_all_mask, \
    bitbang_all_mask)
#else
#error "not supported"
#endif

namespace {

void bitbang_write(uint16_t value, uint16_t rep)
{
  // 繰り返し書き込むことによってhold時間を作る
  for (uint16_t i = 0; i < rep; ++i) {
    bitbang_port_write(value);
  }
  DBG(Serial.print("write "));
  DBG(Serial.println(value, HEX));
}

void bitbang_write_reset()
{
  bitbang_write(bitbang_last_mask | bitbang_clk_mask | bitbang_data_mask, 1);
}

void bitbang_write_bytes(uint8_t const *data, uint32_t len, int wr_repeat)
{
  uint16_t wval = bitbang_clk_mask; // LAST=0, CLK=1
  for (uint32_t i = 0; i < len; ++i) {
    wval = ((data[i] & 0x0f) << 2u) | bitbang_clk_mask;
    bitbang_write(wval, 1);
      // CLKよりもDATAのほうが必ず先に変化するように、先にDATAだけセット
      // して書き込む
    wval &= ~bitbang_clk_mask;
    bitbang_write(wval, wr_repeat); // CLKを0にする
    if (i == 0) {
      // 最初のnibbleは長時間holdする
      bitbang_write(wval, wr_repeat);
    }
    wval = ((data[i] & 0xf0) >> 2u) | (i + 1 == len ? bitbang_last_mask : 0);
    bitbang_write(wval, 1); // 先にDATAとLASTだけセット
    wval |= bitbang_clk_mask;
    bitbang_write(wval, wr_repeat); // CLKを1にする
  }
  bitbang_write(bitbang_last_mask | bitbang_clk_mask | bitbang_data_mask, 1);
}

void bitbang_setup()
{
  byte pins[] = bitbang_pins;
  for (int i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i) {
    pinMode(pins[i], OUTPUT);
  }
  bitbang_write_reset();
}

}; // anonymous namespace

void
bitbang_4bit_sender::begin(uint16_t delay0)
{
  delay = delay0;
  bitbang_setup();
}

void
bitbang_4bit_sender::send(uint8_t const *data, uint16_t sz)
{
  bitbang_write_bytes(data, sz, delay);
}

