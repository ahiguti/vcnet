
// vim: ts=4:sw=4:noexpandtab

#ifndef VCNET_NET_H
#define VCNET_NET_H

int vcnet_net_init(const char *macaddr, const char *ipaddr);
void vcnet_net_set_recv_callback(void (*cb)(int tcpflag, u32 tag, void const *data, u32 datalen));
void vcnet_net_exec_step(void);
int vcnet_net_connected(void);
void vcnet_net_send_tcp(u32 tag0, u32 tag1, void const *data, u32 datalen);
int vcnet_net_send_large_udp(u32 h0, u32 h1, void const *data, u32 datalen);
int vcnet_net_send_raw_udp(u32 tag, void const *data, u32 datalen);
void vcnet_net_flush_tcp(void);
u32 vcnet_net_get_sndbuf(int tcpflag);
int vcnet_net_is_link_up(void);
u64 vcnet_net_get_last_busy_time(void);
#endif

