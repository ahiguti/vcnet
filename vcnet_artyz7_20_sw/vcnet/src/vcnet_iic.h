
#ifndef VCNET_IIC_H
#define VCNET_IIC_H

u32 vcnet_iic_send(u8 addr, const void *data, u32 datalen);
void vcnet_iic_set_led(u32 value);
void vcnet_iic_set_wd(u32 value);
u32 vcnet_iis_read();
void vcnet_ir_out(const void *data, u32 datalen);
u32 vcnet_hdmi_in_stat_read();
void vcnet_gpio_out(const void *data, u32 datalen);

#endif
