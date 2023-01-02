
set_property -dict { PACKAGE_PIN P16   IOSTANDARD LVCMOS33 } [get_ports { I2C_SCL_0 }]; #IO_L24N_T3_34 Sch=CK_SCL
set_property -dict { PACKAGE_PIN P15   IOSTANDARD LVCMOS33 } [get_ports { I2C_SDA_0 }]; #IO_L24P_T3_34 Sch=CK_SDA

set_property -dict { PACKAGE_PIN R14    IOSTANDARD LVCMOS33 } [get_ports { LED_0[0] }]; #IO_L6N_T0_VREF_34 Sch=LED0
set_property -dict { PACKAGE_PIN P14    IOSTANDARD LVCMOS33 } [get_ports { LED_0[1] }]; #IO_L6P_T0_34 Sch=LED1
set_property -dict { PACKAGE_PIN N16    IOSTANDARD LVCMOS33 } [get_ports { LED_0[2] }]; #IO_L21N_T3_DQS_AD14N_35 Sch=LED2
set_property -dict { PACKAGE_PIN M14    IOSTANDARD LVCMOS33 } [get_ports { LED_0[3] }]; #IO_L23P_T3_35 Sch=LED3

set_property -dict { PACKAGE_PIN U13   IOSTANDARD LVCMOS33 } [get_ports { I2S_LR_0 }]; #IO_L3P_T0_DQS_PUDC_B_34 Sch=CK_IO2
set_property -dict { PACKAGE_PIN V13   IOSTANDARD LVCMOS33 } [get_ports { I2S_DAT_0 }]; #IO_L3N_T0_DQS_34 Sch=CK_IO3
set_property -dict { PACKAGE_PIN V15   IOSTANDARD LVCMOS33 } [get_ports { I2S_BCLK_0 }]; #IO_L10P_T1_34 Sch=CK_IO4

set_property -dict { PACKAGE_PIN N17   IOSTANDARD LVCMOS33 } [get_ports { IR_OUT_0 }]; #IO_L23P_T3_34 Sch=CK_IO13

set_property -dict { PACKAGE_PIN T14   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_SCLK[0] }]; #IO_L5P_T0_34 Sch=CK_IO0
set_property -dict { PACKAGE_PIN U12   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_DOUT[0] }]; #IO_L2N_T0_34 Sch=CK_IO1
set_property -dict { PACKAGE_PIN T15   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_SCLK[1] }]; #IO_L5N_T0_34 Sch=CK_IO5
set_property -dict { PACKAGE_PIN R16   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_DOUT[1] }]; #IO_L19P_T3_34 Sch=CK_IO6
set_property -dict { PACKAGE_PIN U17   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_SCLK[2] }]; #IO_L9N_T1_DQS_34 Sch=CK_IO7
set_property -dict { PACKAGE_PIN V17   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_DOUT[2] }]; #IO_L21P_T3_DQS_34 Sch=CK_IO8
set_property -dict { PACKAGE_PIN V18   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_SCLK[3] }]; #IO_L21N_T3_DQS_34 Sch=CK_IO9
set_property -dict { PACKAGE_PIN T16   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_DOUT[3] }]; #IO_L9P_T1_DQS_34 Sch=CK_IO10
set_property -dict { PACKAGE_PIN R17   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_SCLK[4] }]; #IO_L19N_T3_VREF_34 Sch=CK_IO11
set_property -dict { PACKAGE_PIN P18   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_DOUT[4] }]; #IO_L23N_T3_34 Sch=CK_IO12
