
#include "vcnet_common.h"

int verbose = 0;

u32 vcnet_read32(u32 addr)
{
	volatile u32 *p = (u32 *)addr;
	return *p;
}

void vcnet_write32(u32 addr, u32 value)
{
	volatile u32 *p = (u32 *)addr;
	*p = value;
}

