
#if 1
#include "lwipopts.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include "lwip/udp.h"
#include "lwip/inet.h"
#include "xil_printf.h"
#include "xtime_l.h"

#include "vcnet_net_udp.h"
#include "vcnet_common.h"

extern struct netif server_netif;

static int enable_udp_server = 0;
static struct udp_pcb *s_pcb;
static udp_recv_cb_t udp_recv_cb = NULL;
static ip_addr_t c_addr;
static u16_t c_port = 0;

static void vcnet_udp_close(struct udp_pcb *pcb)
{
	if (pcb != NULL) {
		udp_remove(pcb);
	}
}

void vcnet_stop_udp_server()
{
	vcnet_udp_close(s_pcb);
	s_pcb = NULL;
	udp_recv_cb = NULL;
}

static void vcnet_udp_cb(void *arg, struct udp_pcb *pcb, struct pbuf *pb,
	const ip_addr_t *addr, u16_t port)
{
	c_addr = *addr;
	c_port = port;
	if (udp_recv_cb != NULL) {
		(*udp_recv_cb)(pb);
	}
	pbuf_free(pb);
}

void vcnet_start_udp_server()
{
	if (!enable_udp_server) {
		return;
	}
	err_t e;
	if ((s_pcb = udp_new()) == NULL) {
		xil_printf("vcnet_start_udp_server: udp_new failed\r\n");
		vcnet_stop_udp_server();
		return;
	}
	if ((e = udp_bind(s_pcb, IP_ADDR_ANY, VCNET_UDP_PORT)) != ERR_OK) {
		xil_printf("vcnet_start_udp_server: udp_bind failed\r\n");
		vcnet_stop_udp_server();
		return;
	}
	udp_recv(s_pcb, vcnet_udp_cb, NULL);
}

void vcnet_udp_set_recv_cb(udp_recv_cb_t cb)
{
	udp_recv_cb = cb;
}

struct udp_pcb *vcnet_udp_get_pcb()
{
	return s_pcb;
}

static struct pbuf pb_send[2];
static int pb_send_initialized = 0;
static u64 busy_until = 0;

#define VCNET_1G_KCNT (1024ULL * COUNTS_PER_SECOND / 125000000ULL)

int vcnet_udp_busy()
{
	if (!pb_send_initialized) {
		return 0;
	}
	u64 now = vcnet_get_timer_cnt();
	int64_t tdiff = (int64_t)(busy_until - now);
	return (tdiff > 0);
}

int vcnet_udp_send(void const *data0, u32 len0, void const *data1, u32 len1)
{
	if (s_pcb == NULL) {
		return 0;
	}
#if 1
	if (!pb_send_initialized) {
		pb_send[0].next = &pb_send[1];
		pb_send[0].type_internal = PBUF_REF;
		pb_send[0].flags = 0;
		pb_send[0].ref = 1;
		pb_send[0].if_idx = 0; // NETIF_NO_INDEX
		pb_send[1].next = NULL;
		pb_send[1].type_internal = PBUF_REF;
		pb_send[1].flags = 0;
		pb_send[1].ref = 1;
		pb_send[1].if_idx = 0; // NETIF_NO_INDEX
		busy_until = vcnet_get_timer_cnt();
		pb_send_initialized = 1;
	}
	pb_send[0].payload = data0;
	pb_send[0].tot_len = len0 + len1;
	pb_send[0].len = len0;
	pb_send[1].payload = data1;
	pb_send[1].tot_len = len1;
	pb_send[1].len = len1;
	struct pbuf *pb = &pb_send[0];
//	xil_printf("pre payload=%p ref=%u\r\n", pb->payload, pb->ref);
//	u64 t0 = get_time_ms();
	err_t e = udp_sendto(s_pcb, pb, &c_addr, c_port);
	if (e != ERR_OK) {
//		xil_printf("vcnet_udp_send udp_sendto: %d\r\n", e);
//		return e;
	}
//	busy_until = vcnet_get_timer_cnt() + (len0 + len1 + 64) * VCNET_1G_KCNT / 1024;
	busy_until = vcnet_get_timer_cnt() + (len0 + len1 + 64) * VCNET_1G_KCNT * 2 / 1024;
	return e;
//	u64 t1 = get_time_ms();
//	xil_printf("post payload=%p ref=%u %u\r\n", pb->payload, pb->ref, (unsigned)(t1 - t0));
#endif
#if 0
	struct pbuf *pb = pbuf_alloc(PBUF_TRANSPORT, len0 + len1, PBUF_RAM);
	if (pb == NULL) {
//		xil_printf("pbuf_alloc failed\r\n");
		return -1;
	}
//	xil_printf("pre payload=%p ref=%u\r\n", pb0->payload, pb0->ref);
	memcpy(pb->payload, data0, len0);
	memcpy(pb->payload + len0, data1, len1);
	err_t e = udp_sendto(s_pcb, pb, &c_addr, c_port);
	if (e != ERR_OK) {
		 xil_printf("err %d\r\n", e);
		pbuf_free(pb); // FIXME?
		return e;
	}
//	xil_printf("post payload=%p ref=%u\r\n", pb->payload, pb->ref);
	pbuf_free(pb); // FIXME?
	return 0;
#endif
}
#endif
