
#ifndef BITBANG_RECEIVER_H
#define BITBANG_RECEIVER_H

#include <Arduino.h>

struct bitbang_4bit_receiver {
  void begin();
  uint16_t recv(uint8_t *buf, uint16_t buflen,
    unsigned long *start_time_r = nullptr);
};

#endif

