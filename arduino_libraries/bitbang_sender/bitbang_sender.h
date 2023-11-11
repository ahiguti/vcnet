
#ifndef BITBANG_SENDER_H
#define BITBANG_SENDER_H

#include <Arduino.h>

struct bitbang_4bit_sender {
public:
  void begin(uint16_t delay0);
  void send(uint8_t const *data, uint16_t sz);
private:
  uint16_t delay = 10;
};

#endif

