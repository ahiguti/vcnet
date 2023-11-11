
#ifndef VCNET_NET_UDP_H
#define VCNET_NET_UDP_H

#include "lwip/udp.h"

#define VCNET_UDP_PORT 5001

typedef void (*udp_recv_cb_t)(struct pbuf const *);

void vcnet_start_udp_server();
void vcnet_stop_udp_server();
void vcnet_udp_set_recv_cb(udp_recv_cb_t cb);
struct udp_pcb *vcnet_udp_get_pcb();
int vcnet_udp_send(void const *data0, u32 len0, void const *data1, u32 len1);
int vcnet_udp_busy();

#endif
