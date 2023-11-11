#ifndef VCNET_USB_H
#define VCNET_USB_H

#include <Arduino.h>

struct vcnet_usb {
  static void begin(int8_t mcu_id, uint8_t verbose0);
  static void update_regarray(uint16_t const *reg_values, uint16_t nelems);
};

#endif
