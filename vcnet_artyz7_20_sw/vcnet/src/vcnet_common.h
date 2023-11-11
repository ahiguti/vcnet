
#ifndef VCNET_COMMON_H
#define VCNET_COMMON_H

#define VCNET_BYTES_PER_PIXEL 3

#define VCNET_MAC_ADDRESS { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define VCNET_IP_ADDRESS "127.0.0.1"

#include "xil_types.h"

extern int verbose;

u32 vcnet_read32(u32 addr);
void vcnet_write32(u32 addr, u32 value);
u64 vcnet_get_timer_cnt();

#endif
