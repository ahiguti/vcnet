
#ifndef VCNET_IR_H
#define VCNET_IR_H

#include <Arduino.h>

struct vcnet_ir {
  static void begin();
  static void ir_out(uint16_t idx, const void *data, uint16_t len);
};

#endif

