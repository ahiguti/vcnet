
#include <vcnet_ir.h>

#define VCNET_IR_PINS { A0, A1, A2, A3, A4, A5 }
#define VCNET_SERIAL Serial

namespace {

const uint8_t vcnet_ir_pins[] = VCNET_IR_PINS;

void vcnet_ir_setval(uint16_t idx, uint16_t val)
{
  if (idx < sizeof(vcnet_ir_pins) / sizeof(vcnet_ir_pins[0])) {
    digitalWrite(vcnet_ir_pins[idx], val);
  }
}

void vcnet_ir_begin()
{
  VCNET_SERIAL.println("vcnet_ir_setup");
  for (uint8_t i = 0; i < sizeof(vcnet_ir_pins) / sizeof(vcnet_ir_pins[0]);
    ++i) {
    pinMode(vcnet_ir_pins[i], OUTPUT);
    vcnet_ir_setval(i, 1);
  }
}

void vcnet_ir_out(uint16_t idx, const void *data, uint16_t len)
{
  VCNET_SERIAL.print("vcnet_ir_out nbytes=");
  VCNET_SERIAL.println(len);
  uint16_t const *tval = (uint16_t const *)data;
  uint16_t tval_num = len / 2;
  uint16_t val = 0;
  vcnet_ir_setval(idx, val);
  unsigned long prev_time = micros();
  for (uint16_t i = 0; i < tval_num; ++i) {
    uint16_t wt = tval[i];
    while (true) {
      unsigned long now = micros();
      unsigned long tdiff = now - prev_time;
      if (tdiff > wt) {
        break;
      }
    }
    val ^= 1;
    vcnet_ir_setval(idx, val);
    prev_time = prev_time + wt;
  }
  vcnet_ir_setval(idx, 1);
  VCNET_SERIAL.println("vcnet_ir_out done");
}

}; // anonymous namespace

void
vcnet_ir::begin()
{
  vcnet_ir_begin();
}

void
vcnet_ir::ir_out(uint16_t idx, const void *data, uint16_t len)
{
  vcnet_ir_out(idx, data, len);
}

