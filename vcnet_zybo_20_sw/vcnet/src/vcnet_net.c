/*
 * Copyright (C) 2018 - 2019 Xilinx, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

#include "xparameters.h"
#include "netif/xadapter.h"
#include "lwipopts.h"
#include "xil_printf.h"
#include "sleep.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/init.h"
#include "lwip/inet.h"
#include "tcp_perf_server.h"
#if LWIP_IPV6==1
#include "lwip/ip6_addr.h"
#include "lwip/ip6.h"
#else
#include "platform.h"
#include "platform_config.h"

#include "vcnet_net.h"
#include "vcnet_common.h"

#if LWIP_DHCP==1
#include "lwip/dhcp.h"
extern volatile int dhcp_timoutcntr;
#endif
#define DEFAULT_IP_ADDRESS	VCNET_IP_ADDRESS
#define DEFAULT_GW_ADDRESS	VCNET_IP_ADDRESS
  // disable routing
#define DEFAULT_IP_MASK		"255.255.255.0"
#endif /* LWIP_IPV6 */

extern volatile int TcpFastTmrFlag;
extern volatile int TcpSlowTmrFlag;

static char ipaddr_str[16];

void platform_enable_interrupts(void);
void start_application(void);
void print_app_header(void);

#if defined (__arm__) && !defined (ARMR5)
#if XPAR_GIGE_PCS_PMA_SGMII_CORE_PRESENT == 1 || \
		 XPAR_GIGE_PCS_PMA_1000BASEX_CORE_PRESENT == 1
int ProgramSi5324(void);
int ProgramSfpPhy(void);
#endif
#endif

#ifdef XPS_BOARD_ZCU102
#ifdef XPAR_XIICPS_0_DEVICE_ID
int IicPhyReset(void);
#endif
#endif

struct netif server_netif;

#if LWIP_IPV6==1
static void print_ipv6(char *msg, ip_addr_t *ip)
{
	print(msg);
	xil_printf(" %s\n\r", inet6_ntoa(*ip));
}
#else
static void print_ip(char *msg, ip_addr_t *ip)
{
	print(msg);
	xil_printf("%d.%d.%d.%d\r\n", ip4_addr1(ip), ip4_addr2(ip),
			ip4_addr3(ip), ip4_addr4(ip));
}

static void print_ip_settings(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw)
{
	print_ip("Board IP:       ", ip);
	print_ip("Netmask :       ", mask);
	print_ip("Gateway :       ", gw);
}

static void assign_default_ip(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw)
{
	int err;

	xil_printf("Configuring default IP %s \r\n", ipaddr_str);

	err = inet_aton(ipaddr_str, ip);
	if (!err)
		xil_printf("Invalid default IP address: %d\r\n", err);

	err = inet_aton(DEFAULT_IP_MASK, mask);
	if (!err)
		xil_printf("Invalid default IP MASK: %d\r\n", err);

	err = inet_aton(ipaddr_str, gw); // dont route
	if (!err)
		xil_printf("Invalid default gateway address: %d\r\n", err);
}
#endif /* LWIP_IPV6 */

static u64 last_recv_time = 0;
static char recv_buffer[65536];
static u32 recv_buffer_offset = 0;
static void (*recv_callback)(u32 tag, void const *data, u32 datalen) = NULL;

void vcnet_net_set_recv_callback(void (*cb)(u32 tag, void const *data, u32 datalen))
{
	recv_callback = cb;
}

static void vcnet_tcp_recv_cb(struct pbuf const *p)
{
	if (verbose) {
		xil_printf("tcp_recv tot_len=%u\r\n", (unsigned)p->tot_len);
	}
	last_recv_time = get_time_ms();
	if (recv_buffer_offset + p->tot_len >= sizeof(recv_buffer)) {
		xil_printf("tcp_recv drop\r\n");
		return; // drop
	}
	while (p) {
		if (p->len >= 0) {
			if (verbose) {
				xil_printf("tcp_recv len=%u\r\n", (unsigned)p->len);
			}
			memcpy(&recv_buffer[recv_buffer_offset], p->payload, p->len);
			recv_buffer_offset += (u32)(p->len);
			p = p->next;
		}
	}
	char *bp_end = &recv_buffer[0] + recv_buffer_offset;
	char *bp = &recv_buffer[0];
	while (bp + 2 <= bp_end) {
		// format: (datalen:1, typ:1, data:?)
		u32 datalen = bp[0];
		if (bp + 2 + datalen > bp_end) {
			break;
		}
		if (recv_callback != NULL) {
			(*recv_callback)(bp[1], bp + 2, datalen);
		}
		bp += datalen + 2;
	}
	if (bp != &recv_buffer[0]) {
		if (bp != bp_end) {
			memmove(&recv_buffer[0], bp, bp_end - bp);
		}
		recv_buffer_offset -= (bp - &recv_buffer[0]);
	}
}

static struct netif *netif = NULL;

static void reset_ip_addr()
{
	int success = 0;
	while (!success) {
		xil_printf("reset_ip_addr\r\n");
#if (LWIP_IPV6==0)
#if 0 // (LWIP_DHCP==1)
		/* Create a new DHCP client for this interface.
		 * Note: you must call dhcp_fine_tmr() and dhcp_coarse_tmr() at
		 * the predefined regular intervals after starting the client.
		 */
		dhcp_start(netif);
		dhcp_timoutcntr = 24;
		while (((netif->ip_addr.addr) == 0) && (dhcp_timoutcntr > 0))
			xemacif_input(netif);

		if (dhcp_timoutcntr <= 0) {
			if ((netif->ip_addr.addr) == 0) {
				xil_printf("ERROR: DHCP request timed out\r\n");
				assign_default_ip(&(netif->ip_addr),
						&(netif->netmask), &(netif->gw));
			}
			success = 0;
		} else {
			success = 1;
		}

		/* print IP address, netmask and gateway */
#else
		assign_default_ip(&(netif->ip_addr), &(netif->netmask), &(netif->gw));
		success = 1;
#endif
		print_ip_settings(&(netif->ip_addr), &(netif->netmask), &(netif->gw));
#endif /* LWIP_IPV6 */
	}
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
	/* the mac address of the board. this should be unique per board */
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
#if defined (__arm__) && !defined (ARMR5)
#if XPAR_GIGE_PCS_PMA_SGMII_CORE_PRESENT == 1 || \
		XPAR_GIGE_PCS_PMA_1000BASEX_CORE_PRESENT == 1
	ProgramSi5324();
	ProgramSfpPhy();
#endif
#endif

	/* Define this board specific macro in order perform PHY reset
	 * on ZCU102
	 */
#ifdef XPS_BOARD_ZCU102
	IicPhyReset();
#endif

	xil_printf("\r\n\r\n");
	xil_printf("-----lwIP RAW Mode TCP Server Application-----\r\n");

	/* initialize lwIP */
	lwip_init();

	/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(netif, NULL, NULL, NULL, mac_ethernet_address,
				PLATFORM_EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\r\n");
		return -1;
	}

#if LWIP_IPV6==1
	netif->ip6_autoconfig_enabled = 1;
	netif_create_ip6_linklocal_address(netif, 1);
	netif_ip6_addr_set_state(netif, 0, IP6_ADDR_VALID);
	print_ipv6("\n\rlink local IPv6 address is:", &netif->ip6_addr[0]);
#endif /* LWIP_IPV6 */

	netif_set_default(netif);

#if 0
	/* now enable interrupts */
	platform_setup_timer_intr(); // re-initialize timer intr handler
	platform_enable_interrupts();
#endif

	last_recv_time = get_time_ms();

	/* specify that the network if is up */
	netif_set_up(netif);

	reset_ip_addr();

	xil_printf("\r\n");

	/* print app header */
	print_app_header();

	/* start the application*/
	start_application();
	xil_printf("\r\n");

	set_tcp_recv_cb(vcnet_tcp_recv_cb);

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
	struct tcp_pcb *pcb = get_pcb();
	if (pcb != NULL) {
		u64 now = get_time_ms();
		if (last_recv_time + 10000 < now) {
			xil_printf("recv timeout\r\n");
			tcp_server_err(pcb, ERR_TIMEOUT);
		} else {
			conn_state = 1;
		}
	} else {
		if (conn_state == 1) {
			reset_ip_addr();
		}
		conn_state = 0;
		last_recv_time = get_time_ms();
	}
}

int vcnet_net_connected(void)
{
	return get_pcb() != NULL;
}

u32 vcnet_net_send_buffer(void)
{
	struct tcp_pcb *pcb = get_pcb();
	if (pcb == NULL) {
		return 0;
	}
	return tcp_sndbuf(pcb);
}

void vcnet_net_send_data(u32 tag0, u32 tag1, void const *data, u32 datalen)
{
	struct tcp_pcb *pcb = get_pcb();
	if (pcb == NULL) {
		return;
	}
	if ((datalen & 0xff000000) != 0) {
		xil_printf("vcnet_net_send_data: invalid datalen %u\n", (unsigned)datalen);
		return;
	}
	if ((tag0 & 0xffffff00) != 0) {
		xil_printf("vcnet_net_send_data: invalid datalen %u\n", (unsigned)datalen);
		return;
	}
	u32 header[2];
	header[0] = datalen | (tag0 << 24u);
	header[1] = tag1;
	err_t e = tcp_write(pcb, &header[0], 8, TCP_WRITE_FLAG_COPY /* | TCP_WRITE_FLAG_MORE */);
	if (e != ERR_OK) {
		xil_printf("tcp_write: %d\r\n", (int)e);
		tcp_server_err(pcb, e);
	} else {
		u8_t flags = 0 /* TCP_WRITE_FLAG_MORE */;
		err_t e = tcp_write(pcb, data, datalen, flags);
		if (e != ERR_OK) {
			xil_printf("tcp_write: %d\r\n", (int)e);
			tcp_server_err(pcb, e);
		} else {
			tcp_output(pcb);
		}
	}
}

