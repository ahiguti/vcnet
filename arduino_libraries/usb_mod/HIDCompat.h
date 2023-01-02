
#ifndef HIDCompat_H
#define HIDCompat_H

#define _USING_DYNAMIC_HID

#ifndef ARDUINO_ARCH_RP2040

#include "DynamicHID.h"

#else

#include "PluggableUSBHID.h"

// HID

struct HIDSubDescriptor {
  HIDSubDescriptor(const void *data0, const uint16_t len0,
    const bool ipm0 = true)
    : data(data0), len(len0), ipm(ipm0) { }
  const void *const data = nullptr;
  const uint16_t len = 0;
  const bool ipm = true;;
  HIDSubDescriptor *next = nullptr;
};

struct HIDCompat : public USBHID {
  HIDCompat();
  virtual ~HIDCompat();
  int begin();
  void AppendDescriptor(HIDSubDescriptor *subdesc);
  void SendReport(uint8_t id, const void *data, int len);
  virtual const uint8_t *report_desc();
  virtual uint16_t report_desc_length();
};

HIDCompat& HID();

// DynamicHID

static inline HIDCompat& DynamicHID() { return HID(); }
typedef HIDSubDescriptor DynamicHIDSubDescriptor;

#endif

#endif
