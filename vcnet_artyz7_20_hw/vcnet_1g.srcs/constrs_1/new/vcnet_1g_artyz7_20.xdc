set_property -dict { PACKAGE_PIN P19   IOSTANDARD TMDS_33  } [get_ports { TMDS_IN_clk_n }];
set_property -dict { PACKAGE_PIN W20   IOSTANDARD TMDS_33  } [get_ports { TMDS_IN_data_n[0]}];
set_property -dict { PACKAGE_PIN V20   IOSTANDARD TMDS_33  } [get_ports { TMDS_IN_data_p[0]}];
set_property -dict { PACKAGE_PIN U20   IOSTANDARD TMDS_33  } [get_ports { TMDS_IN_data_n[1]}];
set_property -dict { PACKAGE_PIN T20   IOSTANDARD TMDS_33  } [get_ports { TMDS_IN_data_p[1]}];
set_property -dict { PACKAGE_PIN P20   IOSTANDARD TMDS_33  } [get_ports { TMDS_IN_data_n[2]}];
set_property -dict { PACKAGE_PIN N20   IOSTANDARD TMDS_33  } [get_ports { TMDS_IN_data_p[2]}];
set_property -dict { PACKAGE_PIN L17   IOSTANDARD TMDS_33  } [get_ports { TMDS_OUT_clk_n}];
set_property -dict { PACKAGE_PIN L16   IOSTANDARD TMDS_33  } [get_ports { TMDS_OUT_clk_p}];
set_property -dict { PACKAGE_PIN K18   IOSTANDARD TMDS_33  } [get_ports { TMDS_OUT_data_n[0]}];
set_property -dict { PACKAGE_PIN K17   IOSTANDARD TMDS_33  } [get_ports { TMDS_OUT_data_p[0]}];
set_property -dict { PACKAGE_PIN J19   IOSTANDARD TMDS_33  } [get_ports { TMDS_OUT_data_n[1]}];
set_property -dict { PACKAGE_PIN K19   IOSTANDARD TMDS_33  } [get_ports { TMDS_OUT_data_p[1]}];
set_property -dict { PACKAGE_PIN H18   IOSTANDARD TMDS_33  } [get_ports { TMDS_OUT_data_n[2]}];
set_property -dict { PACKAGE_PIN J18   IOSTANDARD TMDS_33  } [get_ports { TMDS_OUT_data_p[2]}];
set_property -dict { PACKAGE_PIN T19   IOSTANDARD LVCMOS33 } [get_ports { HDMI_HPD_tri_o[0]}];
set_property -dict { PACKAGE_PIN U14   IOSTANDARD LVCMOS33 } [get_ports { DDC_scl_io}];
set_property -dict { PACKAGE_PIN U15   IOSTANDARD LVCMOS33 } [get_ports { DDC_sda_io}];

set_property -dict { PACKAGE_PIN R14    IOSTANDARD LVCMOS33 } [get_ports { LED[0] }];
set_property -dict { PACKAGE_PIN P14    IOSTANDARD LVCMOS33 } [get_ports { LED[1] }];
set_property -dict { PACKAGE_PIN N16    IOSTANDARD LVCMOS33 } [get_ports { LED[2] }];
set_property -dict { PACKAGE_PIN M14    IOSTANDARD LVCMOS33 } [get_ports { LED[3] }];

set_property -dict { PACKAGE_PIN U13   IOSTANDARD LVCMOS33 } [get_ports { I2S_LR }];
set_property -dict { PACKAGE_PIN V13   IOSTANDARD LVCMOS33 } [get_ports { I2S_DAT }];
set_property -dict { PACKAGE_PIN V15   IOSTANDARD LVCMOS33 } [get_ports { I2S_BCLK }];

set_property -dict { PACKAGE_PIN N17   IOSTANDARD LVCMOS33 } [get_ports { IR_OUT }];

# ArtyZ7-20: CK_A5 - CK_A2
set_property -dict { PACKAGE_PIN U10   IOSTANDARD LVCMOS33 } [get_ports { GPIO_OUT[0] }];
set_property -dict { PACKAGE_PIN T5    IOSTANDARD LVCMOS33 } [get_ports { GPIO_OUT[1] }];
set_property -dict { PACKAGE_PIN V11   IOSTANDARD LVCMOS33 } [get_ports { GPIO_OUT[2] }];
set_property -dict { PACKAGE_PIN W11   IOSTANDARD LVCMOS33 } [get_ports { GPIO_OUT[3] }];
# ArtyZ7-10 CK_SS, CK_SCK, CK_MISO, CK_MOSI
#set_property -dict { PACKAGE_PIN F16   IOSTANDARD LVCMOS33 } [get_ports { GPIO_OUT[0] }];
#set_property -dict { PACKAGE_PIN H15   IOSTANDARD LVCMOS33 } [get_ports { GPIO_OUT[1] }];
#set_property -dict { PACKAGE_PIN W15   IOSTANDARD LVCMOS33 } [get_ports { GPIO_OUT[2] }];
#set_property -dict { PACKAGE_PIN T12   IOSTANDARD LVCMOS33 } [get_ports { GPIO_OUT[3] }];

set_property -dict { PACKAGE_PIN T14   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_SCLK[0] }];
set_property -dict { PACKAGE_PIN U12   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_DOUT[0] }];
set_property -dict { PACKAGE_PIN T15   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_SCLK[1] }];
set_property -dict { PACKAGE_PIN R16   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_DOUT[1] }];
set_property -dict { PACKAGE_PIN U17   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_SCLK[2] }];
set_property -dict { PACKAGE_PIN V17   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_DOUT[2] }];
set_property -dict { PACKAGE_PIN V18   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_SCLK[3] }];
set_property -dict { PACKAGE_PIN T16   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_DOUT[3] }];
set_property -dict { PACKAGE_PIN R17   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_SCLK[4] }];
set_property -dict { PACKAGE_PIN P18   IOSTANDARD LVCMOS33 } [get_ports { EXT_MCU_DOUT[4] }];
