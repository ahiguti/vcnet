
#ifndef VCNET_NET_H
#define VCNET_NET_H

int vcnet_net_init(const char *macaddr, const char *ipaddr);
void vcnet_net_set_recv_callback(void (*cb)(u32 tag, void const *data, u32 datalen));
void vcnet_net_exec_step(void);
int vcnet_net_connected(void);
void vcnet_net_send_data(u32 tag0, u32 tag1, void const *data, u32 datalen);
u32 vcnet_net_send_buffer(void);

#endif

