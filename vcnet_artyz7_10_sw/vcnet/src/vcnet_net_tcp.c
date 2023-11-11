
// vim: ts=4:sw=4:noexpandtab

#include "lwipopts.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include "lwip/tcp.h"
#include "lwip/inet.h"
#include "xil_printf.h"
#include "platform.h"

#include "vcnet_net_tcp.h"

extern struct netif server_netif;
static struct tcp_pcb *s_pcb, *l_pcb, *c_pcb;
static tcp_recv_cb_t tcp_recv_cb = NULL;

static void vcnet_tcp_close()
{
	if (c_pcb == NULL) {
		return;
	}
	tcp_recv(c_pcb, NULL);
	tcp_err(c_pcb, NULL);
	err_t err = tcp_close(c_pcb);
	if (err != ERR_OK) {
		/* Free memory with abort */
		tcp_abort(c_pcb);
	}
	c_pcb = NULL;
}

void vcnet_tcp_err(void *arg, err_t err)
{
	xil_printf("vcnet_tcp_err: %d\r\n", err);
	vcnet_tcp_close();
}

static void vcnet_tcp_err_cb(void *arg, err_t err)
{
	vcnet_tcp_err(arg, err);
}

void vcnet_tcp_set_recv_cb(tcp_recv_cb_t cb)
{
	tcp_recv_cb = cb;
}

static err_t vcnet_tcp_recv_cb(void *arg, struct tcp_pcb *pcb,
  struct pbuf *pb, err_t err)
{
	if (pb != NULL && pcb == c_pcb && tcp_recv_cb != NULL) {
		(*tcp_recv_cb)(pb);
	}
	if (pb != NULL) {
		tcp_recved(pcb, pb->tot_len);
	}
	if (pb == NULL) {
		if (pcb == c_pcb) {
			xil_printf("vcnet_tcp_recv_cb closed\r\n");
			vcnet_tcp_close();
		}
	} else {
		pbuf_free(pb);
	}
	if (pcb != c_pcb) {
		tcp_abort(pcb);
		xil_printf("vcnet_tcp_recv_cb unknown pcb\r\n");
	}
	return ERR_OK;
}

static err_t vcnet_tcp_accept_cb(void *arg, struct tcp_pcb *npcb, err_t err)
{
	xil_printf("accept_cb %d\r\n", err);
	if ((err != ERR_OK) || (npcb == NULL)) {
		return ERR_VAL;
	}
	if (c_pcb != NULL && c_pcb != npcb) {
		vcnet_tcp_close();
	}
	c_pcb = npcb;
	tcp_nagle_disable(c_pcb);
	tcp_arg(c_pcb, NULL);
	tcp_recv(c_pcb, vcnet_tcp_recv_cb);
	tcp_err(c_pcb, vcnet_tcp_err_cb);
	return ERR_OK;
}

void vcnet_stop_tcp_server()
{
	if (c_pcb != NULL) {
		tcp_abort(c_pcb);
		c_pcb = NULL;
	}
	if (l_pcb != NULL) {
		tcp_abort(l_pcb);
		l_pcb = NULL;
	}
	if (s_pcb != NULL) {
		tcp_abort(s_pcb);
		s_pcb = NULL;
	}
	tcp_recv_cb = NULL;
}

void vcnet_start_tcp_server()
{
	err_t e;
	vcnet_stop_tcp_server();
	if ((s_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY)) == NULL) {
		xil_printf("vcnet_start_tcp_server: tcp_new_ip_type failed\r\n");
		vcnet_stop_tcp_server();
		return;
	}
	if ((e = tcp_bind(s_pcb, IP_ADDR_ANY, VCNET_TCP_PORT)) != ERR_OK) {
		xil_printf("vcnet_start_tcp_server: tcp_bind: %d\r\n", e);
		vcnet_stop_tcp_server();
		return;
	}
	if ((l_pcb = tcp_listen_with_backlog(s_pcb, 1)) == NULL) {
		xil_printf("vcnet_start_tcp_server: tcp_listen_with_backlog failed\r\n");
		vcnet_stop_tcp_server();
		return;
	}
	tcp_arg(l_pcb, NULL);
	tcp_accept(l_pcb, vcnet_tcp_accept_cb);
	xil_printf("vcnet_start_tcp_server\r\n");
}

struct tcp_pcb *vcnet_tcp_get_pcb()
{
	return c_pcb;
}
