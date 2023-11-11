
// vim: ts=4:sw=4:noexpandtab

#include "xparameters.h"
#include "netif/xadapter.h"
#include "lwipopts.h"
#include "xil_printf.h"
#include "sleep.h"
#include "lwip/priv/tcp_priv.h"
//#include "lwip/netif.h"
#include "lwip/init.h"
#include "lwip/inet.h"

#include "platform.h"
#include "platform_config.h"

#include "vcnet_net.h"
#include "vcnet_net_tcp.h"
#include "vcnet_net_udp.h"
#include "vcnet_common.h"

#define DEFAULT_IP_ADDRESS	VCNET_IP_ADDRESS
#define DEFAULT_GW_ADDRESS	VCNET_IP_ADDRESS
  // disable routing
#define DEFAULT_IP_MASK		"255.255.255.0"

extern volatile int TcpFastTmrFlag; // platform.c
extern volatile int TcpSlowTmrFlag; // platform.c

extern enum ethernet_link_status eth_link_status; // ports/xilinx/netif/xadapter.c

static char ipaddr_str[16];

struct netif server_netif;

static void vcnet_print_ip(char *s, ip_addr_t *ip)
{
  xil_printf("%s %d.%d.%d.%d\r\n", s, ip4_addr1(ip), ip4_addr2(ip),
    ip4_addr3(ip), ip4_addr4(ip));
}

static u64 last_tcp_recv_time = 0;
static u64 last_udp_recv_time = 0;
static u64 last_busy_time = 0;
static char tcp_recv_buffer[65536];
static u32 tcp_recv_buffer_offset = 0;
#if 0
static char udp_recv_buffer[65536];
#endif
static int udp_connected = 0;
static void (*recv_callback)(int tcpflag, u32 tag, void const *data, u32 datalen) = NULL;
static struct netif *netif = NULL;

void vcnet_net_set_recv_callback(void (*cb)(int tcpflag, u32 tag, void const *data, u32 datalen))
{
	recv_callback = cb;
}

#if 0
static void vcnet_udp_recv_cb(struct pbuf const *p)
{
//	xil_printf("udp_recv tot_len=%u\r\n", (unsigned)p->tot_len); // FIXME
	last_udp_recv_time = get_time_ms();
	udp_connected = 1;
	u32 udp_recv_buffer_offset = 0;
	while (p) {
		if (p->len >= 0) {
			if (verbose) {
				xil_printf("udp_recv len=%u\r\n", (unsigned)p->len);
			}
			memcpy(&udp_recv_buffer[udp_recv_buffer_offset], p->payload, p->len);
			udp_recv_buffer_offset += (u32)(p->len);
			p = p->next;
		}
	}
	char *bp_end = &udp_recv_buffer[0] + udp_recv_buffer_offset;
	char *bp = &udp_recv_buffer[0];
	while (bp + 2 <= bp_end) {
		// format: (datalen:1, typ:1, data:?)
		u32 datalen = bp[0];
		if (bp + 2 + datalen > bp_end) {
			break;
		}
		if (recv_callback != NULL) {
			(*recv_callback)(0, bp[1], bp + 2, datalen);
		}
		bp += datalen + 2;
	}
}
#endif

static void vcnet_tcp_recv_cb(struct pbuf const *p0)
{
	if (p0 == NULL) {
		// tcp closed
		tcp_recv_buffer_offset = 0;
		return;
	}
//	xil_printf("vcnet_tcp_recv_cb pbuf=%p ref=%u\r\n", p0, p0->ref);
	if (verbose) {
		xil_printf("tcp_recv tot_len=%u\r\n", (unsigned)p0->tot_len);
	}
	last_tcp_recv_time = get_time_ms();
	if (tcp_recv_buffer_offset + p0->tot_len >= sizeof(tcp_recv_buffer)) {
		xil_printf("tcp_recv drop\r\n");
		return; // drop
	}
	struct pbuf const *p = p0;
	while (p) {
		if (p->len >= 0) {
			if (verbose) {
				xil_printf("tcp_recv len=%u\r\n", (unsigned)p->len);
			}
			memcpy(&tcp_recv_buffer[tcp_recv_buffer_offset], p->payload, p->len);
			tcp_recv_buffer_offset += (u32)(p->len);
			p = p->next;
		}
	}
	char *bp_end = &tcp_recv_buffer[0] + tcp_recv_buffer_offset;
	char *bp = &tcp_recv_buffer[0];
	while (bp + 2 <= bp_end) {
		// format: (datalen:1, typ:1, data:?)
		u32 datalen = bp[0];
		if (bp + 2 + datalen > bp_end) {
			break;
		}
		if (recv_callback != NULL) {
			(*recv_callback)(1, bp[1], bp + 2, datalen);
		}
		bp += datalen + 2;
	}
	if (bp != &tcp_recv_buffer[0]) {
		if (bp != bp_end) {
			memmove(&tcp_recv_buffer[0], bp, bp_end - bp);
		}
		tcp_recv_buffer_offset -= (bp - &tcp_recv_buffer[0]);
	}
}

static void vcnet_assign_ip_addr()
{
	xil_printf("vcnet_assgn_ip_addr %s\r\n", ipaddr_str);
	int e = 0;
	if ((e = inet_aton(ipaddr_str, &(netif->ip_addr))) == 0) {
	xil_printf("vcnet_assign_ip_addr: inet_aton: %s %d\r\n",
		ipaddr_str, e);
	}
	if ((e = inet_aton(DEFAULT_IP_MASK, &netif->netmask)) == 0) {
	xil_printf("vcnet_assign_ip_addr: inet_aton: %s %d\r\n",
		DEFAULT_IP_MASK, e);
	}
	if ((e = inet_aton(ipaddr_str, &netif->gw)) == 0) { // dont route
	xil_printf("vcnet_assign_ip_addr: inet_aton: %s %d\r\n",
		ipaddr_str, e);
	}
	vcnet_print_ip("ipaddr", &netif->ip_addr);
	vcnet_print_ip("mask", &netif->netmask);
	vcnet_print_ip("gw", &netif->gw);
//	xil_printf("dbg_lwip_get_mem_count %u %u\r\n", dbg_lwip_get_mem_count(), dbg_lwip_get_memp_count());
}

static int conn_state = 0;

static unsigned char from_hexchar(const char ch)
{
	if (ch >= '0' && ch <= '9') {
		return ch - '0';
	}
	if (ch >= 'a' && ch <= 'f') {
		return ch - 'a' + 10;
	}
	if (ch >= 'A' && ch <= 'F') {
		return ch - 'A' + 10;
	}
	return 0xff;
}

static int set_macaddr(const char *s, unsigned char *bin)
{
	if (strlen(s) != 17) {
		return -1;
	}
	unsigned char b[6];
	for (unsigned i = 0; i < 6; ++i) {
		if (i != 0) {
			if (s[i * 3 - 1] != ':') {
				return -1;
			}
		}
		unsigned h = from_hexchar(s[i * 3]);
		unsigned l = from_hexchar(s[i * 3 + 1]);
		if (h == 0xff || l == 0xff) {
			return -1;
		}
		b[i] = h << 4 | l;
	}
	memcpy(bin, b, 6);
	return 0;
}

int vcnet_net_init(const char *macaddr, const char *ipaddr)
{
	unsigned char mac_ethernet_address[] = VCNET_MAC_ADDRESS;
	if (macaddr != NULL) {
		if (set_macaddr(macaddr, mac_ethernet_address) != 0) {
			xil_printf("malformed mac address: %s\r\n", macaddr);
		}
	}
	{
		unsigned char *p = mac_ethernet_address;
		xil_printf("macaddr=%02x:%02x:%02x:%02x:%02x:%02x\r\n",
			p[0], p[1], p[2], p[3], p[4], p[5]);
	}
	if (ipaddr == NULL) {
		ipaddr = DEFAULT_IP_ADDRESS;
	}
	{
		size_t len = strlen(ipaddr);
		if (len > sizeof(ipaddr_str) - 1) {
			len = sizeof(ipaddr_str) - 1;
		}
		memcpy(ipaddr_str, ipaddr, len);
	}
	netif = &server_netif;
	lwip_init();
	if (!xemac_add(netif, NULL, NULL, NULL, mac_ethernet_address,
				PLATFORM_EMAC_BASEADDR)) {
		xil_printf("vcnet_net_init xemac_add failed\r\n");
		return -1;
	}
	netif_set_default(netif);
	last_tcp_recv_time = get_time_ms();
	last_udp_recv_time = get_time_ms() - 10 * 1000;
	last_busy_time = get_time_ms();
	netif_set_up(netif);
	xil_printf("vcnet_net_init netif_set_up done\r\n");
	if (eth_link_status != ETH_LINK_UP) {
		xil_printf("vcnet_net_init link down\r\n");
	}
	vcnet_assign_ip_addr();
	vcnet_start_tcp_server();
	vcnet_tcp_set_recv_cb(vcnet_tcp_recv_cb);
	#if 0
	vcnet_start_udp_server();
	vcnet_udp_set_recv_cb(vcnet_udp_recv_cb);
	#endif
	return 0;
}

void vcnet_net_exec_step(void)
{
	if (TcpFastTmrFlag) {
		tcp_fasttmr();
		TcpFastTmrFlag = 0;
	}
	if (TcpSlowTmrFlag) {
		tcp_slowtmr();
		TcpSlowTmrFlag = 0;
	}
	xemacif_input(netif);
	struct tcp_pcb *tpcb = vcnet_tcp_get_pcb();
	u64 now = get_time_ms();
	if (tpcb != NULL) {
		// tcp connected
		if (last_tcp_recv_time + 10000 < now) {
			xil_printf("recv timeout\r\n");
			vcnet_tcp_err(tpcb, ERR_TIMEOUT);
			last_tcp_recv_time = get_time_ms();
		} else {
			conn_state = 1;
		}
	} else {
		// tcp not connected
		last_tcp_recv_time = get_time_ms();
	}
	if (last_udp_recv_time + 10000 < now) {
		udp_connected = 0;
	} else {
		conn_state = 1;
	}
	if (tpcb == NULL && udp_connected == 0) {
		if (conn_state == 1) {
			/*
			vcnet_assign_ip_addr();
			*/
		}
		conn_state = 0;
	}
	if (conn_state == 1) {
		last_busy_time = now;
	}
}

int vcnet_net_connected(void)
{
	return vcnet_tcp_get_pcb() != NULL || udp_connected;
}

u64 vcnet_net_get_last_busy_time(void)
{
	return last_busy_time;
}

u32 vcnet_net_get_sndbuf(int tcpflag)
{
	if (tcpflag) {
		struct tcp_pcb *pcb = vcnet_tcp_get_pcb();
		if (pcb == NULL) {
			return 0;
		}
		return tcp_sndbuf(pcb);
	} else {
		struct udp_pcb *pcb = vcnet_udp_get_pcb();
		if (pcb == NULL) {
			return 0;
		}
		return vcnet_udp_busy() ? 0 : 65535;
	}
}

int vcnet_net_send_raw_udp(u32 tag, void const *data, u32 datalen)
{
	if (datalen >= 256 || tag >= 256) {
		return 0;
	}
	char vcnhdr[2];
	vcnhdr[0] = datalen;
	vcnhdr[1] = tag;
	// FIXME: SLOW?
	return vcnet_udp_send(&vcnhdr[0], 2, data, datalen);
#if 0
	udp_send_buffer[0] = datalen;
	udp_send_buffer[1] = tag;
	memcpy(&udp_send_buffer[2], data, datalen);
	vcnet_udp_send(&udp_send_buffer[0], datalen + 2);
#endif
//	xil_printf("vcnet_net_send_raw_udp tag=%u datalen=%u\r\n", (unsigned)tag, (unsigned)datalen);
}

void vcnet_net_send_tcp(u32 tag0, u32 tag1, void const *data, u32 datalen)
{
	struct tcp_pcb *pcb = vcnet_tcp_get_pcb();
	if (pcb == NULL) {
		return;
	}
	if ((datalen & 0xff000000) != 0) {
		xil_printf("vcnet_net_send_data: invalid datalen %u\n", (unsigned)datalen);
		return;
	}
	if ((tag0 & 0xffffff00) != 0) {
		xil_printf("vcnet_net_send_data: invalid tag0 %u\n", (unsigned)tag0);
		return;
	}
	u32 header[2];
	header[0] = datalen | (tag0 << 24u);
	header[1] = tag1;
	err_t e = tcp_write(pcb, &header[0], 8, TCP_WRITE_FLAG_COPY /* | TCP_WRITE_FLAG_MORE */);
	if (e != ERR_OK) {
		xil_printf("tcp_write: %d\r\n", (int)e);
		vcnet_tcp_err(pcb, e);
	} else {
		u8_t flags = 0; /* TCP_WRITE_FLAG_MORE */;
		err_t e = tcp_write(pcb, data, datalen, flags);
		if (e != ERR_OK) {
			xil_printf("tcp_write: %d\r\n", (int)e);
			vcnet_tcp_err(pcb, e);
		}
	}
}

int vcnet_net_send_large_udp(u32 h0, u32 h1, void const *data, u32 datalen)
{
	if (udp_connected == 0) {
		return 0;
	}
	u32 vchdr[2];
	vchdr[0] = h0;
	vchdr[1] = h1;
//	vchdr[0] = (video_line << 16) | (datalen + 8);
//	vchdr[1] = isaudio << 24;
	return vcnet_udp_send(&vchdr[0], 8, data, datalen);
}

void vcnet_net_flush_tcp()
{
	struct tcp_pcb *pcb = vcnet_tcp_get_pcb();
	if (pcb == NULL) {
		return;
	}
	err_t e = tcp_output(pcb);
	if (e != ERR_OK) {
		xil_printf("tcp_output: %d\r\n", (int)e);
		vcnet_tcp_err(pcb, e);
	}
}

int vcnet_net_is_link_up()
{
	return (eth_link_status == ETH_LINK_UP);
}
