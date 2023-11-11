
#include "HIDCompat.h"

#if defined(ARDUINO_ARCH_RP2040)

namespace {

HIDSubDescriptor *desc_list = nullptr;
uint8_t *repdesc = nullptr;
uint16_t repdesc_len = 0;
HID_REPORT report;

};

HIDCompat&
HID()
{
  static HIDCompat hidimpl_instance;
  return hidimpl_instance;
}

HIDCompat::HIDCompat()
{
}

HIDCompat::~HIDCompat()
{
}

int
HIDCompat::begin()
{
  report_desc();
}

void
HIDCompat::AppendDescriptor(HIDSubDescriptor *subdesc)
{
  subdesc->next = desc_list;
  desc_list = subdesc;
}

void
HIDCompat::SendReport(uint8_t id, const void *data, int len)
{
  if (len + 1 > sizeof(report.data)) {
    return;
  }
  report.data[0] = id;
  memcpy(&report.data[1], data, len);
  report.length = len + 1;
  this->send(&report);
}

const uint8_t *
HIDCompat::report_desc()
{
  if (repdesc != nullptr) {
    return repdesc;
  }
  repdesc_len = 0;
  for (HIDSubDescriptor *n = desc_list; n != nullptr; n = n->next) {
    repdesc_len += n->len;
  }
  repdesc = new uint8_t[repdesc_len];
  uint8_t offset = 0;
  for (HIDSubDescriptor *n = desc_list; n != nullptr; n = n->next) {
    memcpy(repdesc + offset, n->data, n->len);
    offset += n->len;
  }
  return repdesc;
}

uint16_t
HIDCompat::report_desc_length()
{
  report_desc();
  return repdesc_len;
}

#endif

