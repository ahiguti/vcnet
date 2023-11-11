
#include <bitbang_receiver.h>

#define DBG(x)

#if defined(ARDUINO_ARCH_AVR)
// Atmega32U4 PortD: { D3, D2, D0, D1, D4, (TXLED), (D12), D6 }
#define bitbang_port_t uint8_t
#define bitbang_last_mask 0x80
#define bitbang_clk_mask 0x10
#define bitbang_read_port(x) { (x) = PIND; }
#define bitbang_get_data(x) ((x) & 0x0f)
#define bitbang_pins { 3, 2, 0, 1, 4, 6 }
#elif defined(ARDUINO_UNOR4_MINIMA) || defined(ARDUINO_UNOR4_WIFI)
// UnoR4 Port01: { (A5/SCL), (A4/SDA), D5, D4, D3, D2, D6, D7 }
#define bitbang_port_t uint16_t
#define bitbang_last_mask 0x80
#define bitbang_clk_mask 0x40
#define bitbang_read_port(x) R_IOPORT_PortRead(nullptr, BSP_IO_PORT_01, &(x))
#define bitbang_get_data(x) ((x) >> 2)
#define bitbang_pins { 5, 4, 3, 2, 6, 7 }
#else
#error "not supported"
#endif

namespace {

uint8_t verbose = 1;

void bitbang_4_recv_setup() {
  uint8_t pins[] = bitbang_pins;
  for (uint16_t i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i) {
    pinMode(pins[i], INPUT_PULLUP);
  }
}

uint16_t bitbang_4_read_bytes(uint8_t *buf_r, uint16_t buflen,
  unsigned long *t_r = nullptr)
{
  noInterrupts();
    // 割り込み禁止したほうが速いCLKでも取りこぼさずに読める。ただし
    // micros()がおかしくなる。
  uint16_t cnt = 50000;
  bitbang_port_t v0;
  bitbang_read_port(v0);
  if ((v0 & (bitbang_last_mask | bitbang_clk_mask)) !=
    (bitbang_last_mask | bitbang_clk_mask)) {
    interrupts();
    return 0; // 開始時点ではLAST=1, CLK=1でなくてはならない
  }
  uint16_t buflen0 = buflen;
  while (buflen > 0) {
    uint8_t byteval = 0;
    do { bitbang_read_port(v0); --cnt; }
      while ((v0 & bitbang_clk_mask) != 0 && cnt != 0);
    if (cnt == 0) {
      break;
    }
    byteval = bitbang_get_data(v0); // 下位4bit
    if (t_r != nullptr) {
      *t_r = micros(); // 開始時刻を返す
      t_r = nullptr;
    }
    do { bitbang_read_port(v0); --cnt; }
      while ((v0 & bitbang_clk_mask) == 0 && cnt != 0);
    if (cnt == 0) {
      break;
    }
    byteval |= bitbang_get_data(v0) << 4; // 上位4bit
    *buf_r = byteval;
    ++buf_r;
    --buflen;
    if ((v0 & bitbang_last_mask) != 0) {
      break;
    }
  }
  interrupts();
  return buflen0 - buflen;
}

}; // anonymous namespace

void
bitbang_4bit_receiver::begin()
{
  bitbang_4_recv_setup();
}

uint16_t
bitbang_4bit_receiver::recv(uint8_t *buf, uint16_t buflen,
  unsigned long *start_time_r)
{
  return bitbang_4_read_bytes(buf, buflen, start_time_r);
}

