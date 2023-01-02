# PMOD JE
# V12 W16 J15 H15 (GND VCC)
# V13 U17 T17 Y17 (GND VCC)
set_property PACKAGE_PIN V12 [get_ports IIC_0_scl_io]
set_property PACKAGE_PIN W16 [get_ports IIC_0_sda_io]
set_property IOSTANDARD LVCMOS33 [get_ports IIC_0_scl_io]
set_property IOSTANDARD LVCMOS33 [get_ports IIC_0_sda_io]
#set_property PULLUP TRUE [get_ports IIC_0_scl_io]
#set_property PULLUP TRUE [get_ports IIC_0_sda_io]


set_property PACKAGE_PIN J15 [get_ports I2C_SCL_0]
set_property PACKAGE_PIN H15 [get_ports I2C_SDA_0]
set_property IOSTANDARD LVCMOS33 [get_ports I2C_SCL_0]
set_property IOSTANDARD LVCMOS33 [get_ports I2C_SDA_0]

set_property -dict { PACKAGE_PIN M14   IOSTANDARD LVCMOS33 } [get_ports { LED_0[0] }]; #IO_L23P_T3_35 Sch=led[0]
set_property -dict { PACKAGE_PIN M15   IOSTANDARD LVCMOS33 } [get_ports { LED_0[1] }]; #IO_L23N_T3_35 Sch=led[1]
set_property -dict { PACKAGE_PIN G14   IOSTANDARD LVCMOS33 } [get_ports { LED_0[2] }]; #IO_0_35 Sch=led[2]
set_property -dict { PACKAGE_PIN D18   IOSTANDARD LVCMOS33 } [get_ports { LED_0[3] }]; #IO_L3N_T0_DQS_AD1N_35 Sch=led[3]

#set_property -dict { PACKAGE_PIN N15   IOSTANDARD LVCMOS33 } [get_ports { ja[0] }]; #IO_L21P_T3_DQS_AD14P_35 Sch=JA1_R_p		   
#set_property -dict { PACKAGE_PIN L14   IOSTANDARD LVCMOS33 } [get_ports { ja[1] }]; #IO_L22P_T3_AD7P_35 Sch=JA2_R_P             
#set_property -dict { PACKAGE_PIN K16   IOSTANDARD LVCMOS33 } [get_ports { ja[2] }]; #IO_L24P_T3_AD15P_35 Sch=JA3_R_P            
#set_property -dict { PACKAGE_PIN K14   IOSTANDARD LVCMOS33 } [get_ports { ja[3] }]; #IO_L20P_T3_AD6P_35 Sch=JA4_R_P             
set_property -dict { PACKAGE_PIN L14   IOSTANDARD LVCMOS33 } [get_ports { I2S_BCLK_0 }];
set_property -dict { PACKAGE_PIN K16   IOSTANDARD LVCMOS33 } [get_ports { I2S_DAT_0 }];
set_property -dict { PACKAGE_PIN K14   IOSTANDARD LVCMOS33 } [get_ports { I2S_LR_0 }];

# PMOD JB
#set_property -dict { PACKAGE_PIN V8    IOSTANDARD LVCMOS33     } [get_ports { jb[0] }]; #IO_L15P_T2_DQS_13 Sch=jb_p[1]		 
#set_property -dict { PACKAGE_PIN W8    IOSTANDARD LVCMOS33     } [get_ports { jb[1] }]; #IO_L15N_T2_DQS_13 Sch=jb_n[1]         
#set_property -dict { PACKAGE_PIN U7    IOSTANDARD LVCMOS33     } [get_ports { jb[2] }]; #IO_L11P_T1_SRCC_13 Sch=jb_p[2]        
#set_property -dict { PACKAGE_PIN V7    IOSTANDARD LVCMOS33     } [get_ports { jb[3] }]; #IO_L11N_T1_SRCC_13 Sch=jb_n[2]        
#set_property -dict { PACKAGE_PIN W8   IOSTANDARD LVCMOS33 } [get_ports { I2S_BCLK_0 }];
#set_property -dict { PACKAGE_PIN U7   IOSTANDARD LVCMOS33 } [get_ports { I2S_DAT_0 }];
#set_property -dict { PACKAGE_PIN V7   IOSTANDARD LVCMOS33 } [get_ports { I2S_LR_0 }];

# PMOD JC
#set_property -dict { PACKAGE_PIN V15   IOSTANDARD LVCMOS33     } [get_ports { jc[0] }]; #IO_L10P_T1_34 Sch=jc_p[1]   			 
#set_property -dict { PACKAGE_PIN W15   IOSTANDARD LVCMOS33     } [get_ports { jc[1] }]; #IO_L10N_T1_34 Sch=jc_n[1]		     
#set_property -dict { PACKAGE_PIN T11   IOSTANDARD LVCMOS33     } [get_ports { jc[2] }]; #IO_L1P_T0_34 Sch=jc_p[2]              
#set_property -dict { PACKAGE_PIN T10   IOSTANDARD LVCMOS33     } [get_ports { jc[3] }]; #IO_L1N_T0_34 Sch=jc_n[2]              
#set_property -dict { PACKAGE_PIN W15   IOSTANDARD LVCMOS33 } [get_ports { I2S_BCLK_0 }];
#set_property -dict { PACKAGE_PIN T11   IOSTANDARD LVCMOS33 } [get_ports { I2S_DAT_0 }];
#set_property -dict { PACKAGE_PIN T10   IOSTANDARD LVCMOS33 } [get_ports { I2S_LR_0 }];
