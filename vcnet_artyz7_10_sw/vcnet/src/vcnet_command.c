#include "vcnet_common.h"
#include "vcnet_command.h"
#include "vcnet_iic.h"
#include <string.h>
#include "xil_printf.h"
#include "platform.h"

typedef struct vcnet_command {
	u8 iic_addr;
	u8 len;
	char buffer[4];
} vcnet_command_t;

#define VCNET_COMMAND_NUM 4096

static vcnet_command_t commands[VCNET_COMMAND_NUM];
static u32 command_idx_wr = 0;
static u32 command_idx_rd = 0;
static u64 last_sent = 0;

void vcnet_command_add(const void *data, u32 len)
{
	if (verbose) {
		xil_printf("vcnet_command_add len=%u\r\n", (unsigned)len);
	}
	if (len < 1 + 1 || len > 4 + 1) {
		return;
	}
	u32 command_idx_wr_next = (command_idx_wr + 1) % VCNET_COMMAND_NUM;
	if (command_idx_wr_next == command_idx_rd) {
		xil_printf("vcnet_command_add drop\r\n");
		return;
	}
	vcnet_command_t *c = &commands[command_idx_wr];
	char *p = (char *)data;
	c->iic_addr = p[0] & 0x7f;
	c->len = len - 1;
	memcpy(&c->buffer[0], p + 1, len - 1);
	command_idx_wr = command_idx_wr_next;
	if (verbose) {
		xil_printf("vcnet_command_add wr=%u\r\n", (unsigned)command_idx_wr);
	}
}

void vcnet_command_exec_step(void)
{
	u64 now = get_time_ms();
	if (now - last_sent < 3) {
		return;
	}
	if (command_idx_rd != command_idx_wr) {
		last_sent = now;
		vcnet_command_t *c = &commands[command_idx_rd];
		if (verbose) {
			xil_printf("iic len=%u\r\n", (unsigned)c->len);
		}
		vcnet_iic_send(c->iic_addr, &c->buffer[0], c->len);
		command_idx_rd = (command_idx_rd + 1) % VCNET_COMMAND_NUM;
	}
}

u32 vcnet_command_num_queued(void)
{
	return (command_idx_wr - command_idx_rd) % VCNET_COMMAND_NUM;
}
