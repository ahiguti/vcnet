
#ifndef HIDCompat_H
#define HIDCompat_H

#define _USING_DYNAMIC_HID

#if defined(ARDUINO_ARCH_AVR)

  // Atmel32u4
  #include "DynamicHIDMod.h"

#elif defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_UNOR4_MINIMA) || \
  defined(ARDUINO_UNOR4_WIFI)

  // Atmel SAMD, Renesus
  #include "HID.h"

  struct DynamicHIDSubDescriptor : public HIDSubDescriptor {
      DynamicHIDSubDescriptor(const void *data0, const uint16_t len0,
        bool ipm0 = true)
        : HIDSubDescriptor(data0, len0) { }
  };

  static inline HID_& DynamicHID() { return HID(); }
  // typedef HIDSubDescriptor DynamicHIDSubDescriptor;

#elif defined(ARDUINO_ARCH_RP2040)

  // Mbed RP2040
  #include "PluggableUSBHID.h"

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

  static inline HIDCompat& DynamicHID() { return HID(); }
  typedef HIDSubDescriptor DynamicHIDSubDescriptor;

#else

  #error "unsuppoted arch"

#endif

#endif
