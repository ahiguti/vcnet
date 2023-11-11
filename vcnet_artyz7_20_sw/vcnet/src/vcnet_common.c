
#include "xtime_l.h"
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

u64 vcnet_get_timer_cnt()
{
	XTime v = 0;
	XTime_GetTime(&v);
	return (u64)v; // COUNT_PER_SECONDS
}
