#include "xil_printf.h"
#include "xparameters.h"
#include "platform.h"
#include "xil_io.h"
#include "vcnet_iic.h"
#include "vcnet_common.h"
#include "xtime_l.h"

#if 0

#include "xiic.h"

#define IIC_BASE_ADDRESS XPAR_IIC_0_BASEADDR

u32 vcnet_iic_send(u8 addr, const void *data, u32 datalen)
{
    u32 sent = 0;
	sent = XIic_Send(IIC_BASE_ADDRESS, addr, (u8 *)data, datalen, XIIC_STOP);
	xil_printf("sent %u\r\n", (unsigned)sent);
	if (sent != datalen) {
		// reset tx fifo
		u32 cr = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
		XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, cr | XIIC_CR_TX_FIFO_RESET_MASK);
		XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, XIIC_CR_ENABLE_DEVICE_MASK);
	}
	return sent;
}

#endif

#define IO_IIC_ADDR 0x43c40000
	// control registers for iic, watchdog-timer, and iis

static void vcnet_iic_wait(void)
{
	volatile u32 *addr = (volatile u32 *)(IO_IIC_ADDR);
//	xil_printf("vcnet_iic_wait\r\n");
	while (1) {
		u32 stat = addr[0];
		if ((stat & 0x80) != 0) {
			break;
		}
	}
//	xil_printf("vcnet_iic_wait done\r\n");
}

u32 vcnet_iic_send(u8 devaddr, const void *data, u32 datalen)
{
	if (datalen > 4) {
		xil_printf("iic_send: ignore datalen=%u\r\n", (unsigned)datalen);
		return 0;
	}
	volatile u32 *addr = (volatile u32 *)(IO_IIC_ADDR);
	vcnet_iic_wait();
	u32 devaddr32 = devaddr;
	u32 data32 = 0;
	memcpy(&data32, data, datalen);
	if (verbose) {
		/* if ((data32 & 0xff) == 0x06) { */
		xil_printf("iic_send %x\r\n", (unsigned)data32);
	}
	addr[2] = data32;
	addr[4] = datalen;
	addr[8] = 0;
	addr[0] = devaddr32;
	u32 sent = 0;
#if 0
	vcnet_iic_wait();
	u32 err = addr[0] & 0x07;
	sent = err != 0 ? 0 : datalen;
	if (verbose || err) {
		xil_printf("sent %u\r\n", (unsigned)sent);
	}
#endif
	return sent;
}

void vcnet_iic_set_led(u32 value)
{
	volatile u32 *addr = (volatile u32 *)(IO_IIC_ADDR);
	addr[10] = value;
}

void vcnet_iic_set_wd(u32 value)
{
	volatile u32 *addr = (volatile u32 *)(IO_IIC_ADDR);
	addr[12] = value;
}

#if 0
void vcnet_iis_start()
{
	volatile u32 *addr = (volatile u32 *)(IO_IIC_ADDR);
	addr[14] = (u32)&iis_buffer[0];
}

void *vcnet_iis_cur_addr()
{
	volatile u32 *addr = (volatile u32 *)(IO_IIC_ADDR);
	return (void *)addr[14];
}

void *vcnet_iis_get_buffer(u32 *sz_r)
{
	if (sz_r != 0) {
		*sz_r = sizeof(iis_buffer);
	}
	return &iis_buffer[0];
}
#endif

u32 vcnet_iis_read()
{
	volatile u32 *addr = (volatile u32 *)(IO_IIC_ADDR);
	(void)addr[13];
	return addr[14];
}

u64 get_time_us()
{
	XTime tCur = 0;
	XTime_GetTime(&tCur);
	return (tCur/(COUNTS_PER_SECOND/1000000));
}

#define IO_IR_ADDR 0x43c60000

void vcnet_ir_setval(u32 v)
{
	volatile u32 *addr = (volatile u32 *)(IO_IR_ADDR);
	while (addr[0] == 0) {
		// busy
		xil_printf("ir_setval busy\r\n");
	}
	addr[0] = v;
	xil_printf("ir_setval %x\r\n", (unsigned)v);
}

void vcnet_ir_init()
{
	vcnet_ir_setval(0); // end-of-frame mark
}

void vcnet_ir_out(const void *data, u32 datalen)
{
	xil_printf("ir_out %u bytes\r\n", (unsigned)datalen);
	u32 const *tval32 = (u32 const *)data;
	u16 const *tval16 = (u16 const *)data;
	u32 tval16_num = datalen / 2;
	vcnet_ir_setval(0);
	{
		for (u32 i = 0; i < tval16_num / 2; ++i) {
			vcnet_ir_setval(tval32[i]);
		}
		u32 v = 0;
		if ((tval16_num & 1) != 0) {
			v = tval16[tval16_num - 1];
		}
		vcnet_ir_setval(v);
	}
	vcnet_ir_setval(0);
}

#if 0
void vcnet_ir_setval(u32 v)
{
	volatile u32 *addr = (volatile u32 *)(IO_IIC_ADDR);
	addr[16] = v;
}

void vcnet_ir_init()
{
	vcnet_ir_setval(1);
}

void vcnet_ir_out(const void *data, u32 datalen)
{
	xil_printf("ir_out %u bytes\r\n", (unsigned)datalen);
	u64 debug_tsum = 0;
	u16 const *tval = (u16 const *)data;
	u32 tval_num = datalen / 2;
	u32 val = 0;
	{
		for (u32 i = 0; i < tval_num; ++i) {
			xil_printf("ir %u\r\n", (unsigned)tval[i]);
		}
	}
	u64 t0_ms = get_time_ms();
	vcnet_ir_setval(val);
	u64 prev_time = get_time_us();
	for (u32 i = 0; i < tval_num; ++i) {
		u64 wt = tval[i];
		while (1) {
			u64 now = get_time_us();
			if (now > prev_time + wt) {
				break;
			}
		}
		val ^= 1;
		vcnet_ir_setval(val);
		prev_time = prev_time + wt;
		debug_tsum += wt;
	}
	vcnet_ir_setval(1);
	u64 t1_ms = get_time_ms();
	xil_printf("ir_out %u ms tsum %u\r\n",
		(unsigned)(t1_ms - t0_ms),
		(unsigned)debug_tsum);
}
#endif

#define IO_HDMI_IN_STAT_ADDR 0x43c70000

u32 vcnet_hdmi_in_stat_read()
{
	volatile u32 *addr = (volatile u32 *)(IO_HDMI_IN_STAT_ADDR);
	return *addr;
}

#define IO_GPIO_ADDR 0x43c60000

static u16 vcnet_gpio_cur_val = 0xffffu;

void vcnet_gpio_setval(u16 val, u16 mask)
{
	vcnet_gpio_cur_val &= ~mask;
	vcnet_gpio_cur_val |= (val & mask);
	volatile u32 *addr = (volatile u32 *)(IO_IIC_ADDR);
	addr[18] = vcnet_gpio_cur_val;
}

void vcnet_gpio_out(const void *data, u32 datalen)
{
	xil_printf("gpio_out %u bytes\r\n", (unsigned)datalen);
	if (datalen < 4) {
		return;
	}
	u16 const *tval16 = (u32 const *)data;
	u16 val = tval16[0];
	u16 mask = tval16[1];
	vcnet_gpio_setval(val, mask);
}
