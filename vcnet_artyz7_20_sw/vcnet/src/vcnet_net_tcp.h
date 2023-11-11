
// vim: ts=4:sw=4:noexpandtab

#ifndef VCNET_NET_TCP_H
#define VCNET_NET_TCP_H

#include "lwip/tcp.h"

#define VCNET_TCP_PORT 5001

typedef void (*tcp_recv_cb_t)(struct pbuf const *);

void vcnet_start_tcp_server();
void vcnet_stop_tcp_server();
void vcnet_tcp_set_recv_cb(tcp_recv_cb_t cb);
struct tcp_pcb *vcnet_tcp_get_pcb();
void vcnet_tcp_err(void *arg, err_t err);

#endif
