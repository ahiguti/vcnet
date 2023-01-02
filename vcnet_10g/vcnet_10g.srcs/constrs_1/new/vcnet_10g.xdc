#################################
set_property PACKAGE_PIN AD26 [get_ports SFP1_RX_LOS]
set_property IOSTANDARD LVCMOS25 [get_ports SFP1_RX_LOS]
set_property PACKAGE_PIN AF24 [get_ports SFP1_TX_DISABLE]
set_property IOSTANDARD LVCMOS25 [get_ports SFP1_TX_DISABLE]

#################################
#set_property PACKAGE_PIN AE23 [get_ports SFP2_TX_DISABLE]
#set_property IOSTANDARD LVCMOS25 [get_ports SFP2_TX_DISABLE]
#set_property PACKAGE_PIN AD25 [get_ports SFP2_RX_LOS]
#set_property IOSTANDARD LVCMOS25 [get_ports SFP2_RX_LOS]

#################################
#set_property PACKAGE_PIN E11 [get_ports PHY1_RX_CLK]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY1_RX_CLK]
#create_clock -period 8.000 -name PHY1_RX_CLK -waveform {0.000 4.000} [get_ports PHY1_RX_CLK]
#set_property PACKAGE_PIN D11 [get_ports PHY1_RX_CTRL]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY1_RX_CTRL]
#set_property PACKAGE_PIN A10 [get_ports {PHY1_RX_D[0]}]
#set_property PACKAGE_PIN B10 [get_ports {PHY1_RX_D[1]}]
#set_property PACKAGE_PIN B11 [get_ports {PHY1_RX_D[2]}]
#set_property PACKAGE_PIN C11 [get_ports {PHY1_RX_D[3]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY1_RX_D[*]}]
#set_property PACKAGE_PIN G10 [get_ports PHY1_GTX_CLK]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY1_GTX_CLK]
#set_property PACKAGE_PIN H8 [get_ports {PHY1_TX_D[0]}]
#set_property PACKAGE_PIN H9 [get_ports {PHY1_TX_D[1]}]
#set_property PACKAGE_PIN J9 [get_ports {PHY1_TX_D[2]}]
#set_property PACKAGE_PIN J10 [get_ports {PHY1_TX_D[3]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY1_TX_D[*]}]
#set_property PACKAGE_PIN G9 [get_ports PHY1_TX_CTRL]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY1_TX_CTRL]
#set_property PACKAGE_PIN D9 [get_ports {PHY1_RESETN[0]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY1_RESETN[0]}]
#set_property PACKAGE_PIN C8 [get_ports PHY1_MDIO]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY1_MDIO]
#set_property PACKAGE_PIN C9 [get_ports PHY1_MDC]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY1_MDC]

#################################
set_property PACKAGE_PIN D16 [get_ports {LED[0]}]
set_property PACKAGE_PIN G16 [get_ports {LED[1]}]
set_property PACKAGE_PIN H16 [get_ports {LED[2]}]
#set_property PACKAGE_PIN E18 [get_ports {LED[3]}]
#set_property PACKAGE_PIN E17 [get_ports {LED[4]}]
#set_property PACKAGE_PIN E16 [get_ports {LED[5]}]
#set_property PACKAGE_PIN H18 [get_ports {LED[6]}]
#set_property PACKAGE_PIN H17 [get_ports {LED[7]}]
set_property IOSTANDARD LVCMOS18 [get_ports {LED[*]}]

#################################
set_property C_CLK_INPUT_FREQ_HZ 300000000 [get_debug_cores dbg_hub]
set_property C_ENABLE_CLK_DIVIDER false [get_debug_cores dbg_hub]
set_property C_USER_SCAN_CHAIN 1 [get_debug_cores dbg_hub]

#################################
set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]
set_property BITSTREAM.CONFIG.CONFIGRATE 50 [current_design]
set_property BITSTREAM.CONFIG.SPI_BUSWIDTH 4 [current_design]

#### AESKU040

set_property PACKAGE_PIN AD20 [get_ports HDMI1_P[0]]; #LA19_P HDMI1_P0
set_property PACKAGE_PIN AC21 [get_ports HDMI1_P[1]]; #LA20_N HDMI1_P1
set_property PACKAGE_PIN AB21 [get_ports HDMI1_P[2]]; #LA20_P HDMI1_P2
set_property PACKAGE_PIN AF13 [get_ports HDMI1_P[3]]; #LA15_N HDMI1_P3
set_property PACKAGE_PIN AE15 [get_ports HDMI1_P[4]]; #LA14_N HDMI1_P4
set_property PACKAGE_PIN AF14 [get_ports HDMI1_P[5]]; #LA15_P HDMI1_P5
set_property PACKAGE_PIN AF15 [get_ports HDMI1_P[6]]; #LA16_N HDMI1_P6
set_property PACKAGE_PIN AE16 [get_ports HDMI1_P[7]]; #LA16_P HDMI1_P7
set_property PACKAGE_PIN AD13 [get_ports HDMI1_P[8]]; #LA11_N HDMI1_P8
set_property PACKAGE_PIN AD15 [get_ports HDMI1_P[9]]; #LA14_P HDMI1_P9
set_property PACKAGE_PIN AD14 [get_ports HDMI1_P[10]]; #LA11_P HDMI1_P10
set_property PACKAGE_PIN AC13 [get_ports HDMI1_P[11]]; #LA12_N HDMI1_P11
set_property PACKAGE_PIN AC14 [get_ports HDMI1_P[12]]; #LA12_P HDMI1_P12
set_property PACKAGE_PIN W13 [get_ports HDMI1_P[13]]; #LA07_N HDMI1_P13
set_property PACKAGE_PIN AB14 [get_ports HDMI1_P[14]]; #LA08_N HDMI1_P14
set_property PACKAGE_PIN W14 [get_ports HDMI1_P[15]]; #LA07_P HDMI1_P15
set_property PACKAGE_PIN AA14 [get_ports HDMI1_P[16]]; #LA08_P HDMI1_P16
set_property PACKAGE_PIN AF10 [get_ports HDMI1_P[17]]; #LA04_N HDMI1_P17
set_property PACKAGE_PIN AE10 [get_ports HDMI1_P[18]]; #LA04_P HDMI1_P18
set_property PACKAGE_PIN AD8 [get_ports HDMI1_P[19]]; #LA03_N HDMI1_P19
set_property PACKAGE_PIN AC8 [get_ports HDMI1_P[20]]; #LA03_P HDMI1_P20
set_property PACKAGE_PIN AB9 [get_ports HDMI1_P[21]]; #LA02_N HDMI1_P21
set_property PACKAGE_PIN AB10 [get_ports HDMI1_P[22]]; #LA02_P HDMI1_P22
set_property PACKAGE_PIN AB11 [get_ports HDMI1_P[23]]; #LA00_N_CC HDMI1_P23
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI1_P[*]}];
set_property PACKAGE_PIN AB19 [get_ports HDMI1_SCLK]; #LA18_P_CC HDMI1_SCLK
set_property IOSTANDARD LVCMOS18 [get_ports HDMI1_SCLK];
set_property PACKAGE_PIN AE20 [get_ports HDMI1_LRCLK]; #LA21_P HDMI1_LRCLK
set_property IOSTANDARD LVCMOS18 [get_ports HDMI1_LRCLK];
#set_property PACKAGE_PIN W20 [get_ports HDMI1_MCLK]; #LA17_P_CC HDMI1_MCLK
#set_property IOSTANDARD LVCMOS18 [get_ports HDMI1_MCLK];
set_property PACKAGE_PIN Y18 [get_ports HDMI1_AP]; #LA23_N HDMI1_AP
set_property IOSTANDARD LVCMOS18 [get_ports HDMI1_AP];
set_property PACKAGE_PIN AC19 [get_ports HDMI1_VS]; #LA22_P HDMI1_VS
set_property IOSTANDARD LVCMOS18 [get_ports HDMI1_VS];
set_property PACKAGE_PIN AD21 [get_ports HDMI1_HS]; #LA19_N HDMI1_HS
set_property IOSTANDARD LVCMOS18 [get_ports HDMI1_HS];
set_property PACKAGE_PIN AD19 [get_ports HDMI1_DE]; #LA22_N HDMI1_DE
set_property IOSTANDARD LVCMOS18 [get_ports HDMI1_DE];
set_property PACKAGE_PIN AB12 [get_ports HDMI1_LLC]; #LA00_P_CC HDMI1_LLC
set_property IOSTANDARD LVCMOS18 [get_ports HDMI1_LLC];
set_property PACKAGE_PIN W18 [get_ports HDMI1_SDA]; #LA25_P HDMI1_SDA
set_property IOSTANDARD LVCMOS18 [get_ports HDMI1_SDA];
set_property PACKAGE_PIN AE21 [get_ports HDMI1_SCL]; #LA21_N HDMI1_SCL
set_property IOSTANDARD LVCMOS18 [get_ports HDMI1_SCL];
#set_property PACKAGE_PIN Y17 [get_ports HDMI1_RESETN]; #LA23_P HDMI1_RESETN
#set_property IOSTANDARD LVCMOS18 [get_ports HDMI1_RESETN];
#set_property PACKAGE_PIN W19 [get_ports HDMI1_INT1]; #LA25_N HDMI1_INT1
#set_property IOSTANDARD LVCMOS18 [get_ports HDMI1_INT1];

#set_property PACKAGE_PIN AD26 [get_ports SFP1_RX_LOS]
#set_property IOSTANDARD LVCMOS25 [get_ports SFP1_RX_LOS]
#set_property PACKAGE_PIN AF24 [get_ports SFP1_TX_DISABLE]
#set_property IOSTANDARD LVCMOS25 [get_ports SFP1_TX_DISABLE]

#set_property PACKAGE_PIN AE23 [get_ports SFP2_TX_DISABLE]
#set_property IOSTANDARD LVCMOS25 [get_ports SFP2_TX_DISABLE]
#set_property PACKAGE_PIN AD25 [get_ports SFP2_RX_LOS]
#set_property IOSTANDARD LVCMOS25 [get_ports SFP2_RX_LOS]

#set_property PACKAGE_PIN E11 [get_ports PHY1_RX_CLK]
#set_property PACKAGE_PIN A10 [get_ports {PHY1_RX_D[0]}]
#set_property PACKAGE_PIN B10 [get_ports {PHY1_RX_D[1]}]
#set_property PACKAGE_PIN B11 [get_ports {PHY1_RX_D[2]}]
#set_property PACKAGE_PIN C11 [get_ports {PHY1_RX_D[3]}]
#set_property PACKAGE_PIN H8 [get_ports {PHY1_TX_D[0]}]
#set_property PACKAGE_PIN H9 [get_ports {PHY1_TX_D[1]}]
#set_property PACKAGE_PIN J9 [get_ports {PHY1_TX_D[2]}]
#set_property PACKAGE_PIN J10 [get_ports {PHY1_TX_D[3]}]
#set_property PACKAGE_PIN G9 [get_ports PHY1_TX_CTRL]
#set_property PACKAGE_PIN D11 [get_ports PHY1_RX_CTRL]
#set_property PACKAGE_PIN D9 [get_ports {PHY1_RESETN[0]}]
#set_property PACKAGE_PIN C8 [get_ports PHY1_MDIO]
#set_property PACKAGE_PIN C9 [get_ports PHY1_MDC]
#set_property PACKAGE_PIN G10 [get_ports PHY1_GTX_CLK]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY1_GTX_CLK]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY1_MDIO]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY1_MDC]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY1_RESETN[0]}]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY1_RX_CTRL]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY1_TX_CTRL]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY1_TX_D[3]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY1_TX_D[2]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY1_TX_D[1]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY1_TX_D[0]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY1_RX_D[3]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY1_RX_D[2]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY1_RX_D[1]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY1_RX_D[0]}]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY1_RX_CLK]

#set_property PACKAGE_PIN E13 [get_ports PHY2_RX_CLK]
#set_property PACKAGE_PIN A14 [get_ports {PHY2_RX_D[0]}]
#set_property PACKAGE_PIN B14 [get_ports {PHY2_RX_D[1]}]
#set_property PACKAGE_PIN A15 [get_ports {PHY2_RX_D[2]}]
#set_property PACKAGE_PIN B15 [get_ports {PHY2_RX_D[3]}]
#set_property PACKAGE_PIN F13 [get_ports {PHY2_TX_D[0]}]
#set_property PACKAGE_PIN F14 [get_ports {PHY2_TX_D[1]}]
#set_property PACKAGE_PIN F15 [get_ports {PHY2_TX_D[2]}]
#set_property PACKAGE_PIN H14 [get_ports {PHY2_TX_D[3]}]
#set_property PACKAGE_PIN G14 [get_ports PHY2_TX_CTRL]
#set_property PACKAGE_PIN E12 [get_ports PHY2_RX_CTRL]
#set_property PACKAGE_PIN E15 [get_ports {PHY2_RESETN[0]}]
#set_property PACKAGE_PIN C14 [get_ports PHY2_MDIO]
#set_property PACKAGE_PIN D14 [get_ports PHY2_MDC]
#set_property PACKAGE_PIN G15 [get_ports PHY2_GTX_CLK]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY2_GTX_CLK]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY2_MDIO]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY2_MDC]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY2_RESETN[0]}]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY2_RX_CTRL]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY2_TX_CTRL]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY2_TX_D[3]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY2_TX_D[2]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY2_TX_D[1]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY2_TX_D[0]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY2_RX_D[3]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY2_RX_D[2]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY2_RX_D[1]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {PHY2_RX_D[0]}]
#set_property IOSTANDARD LVCMOS18 [get_ports PHY2_RX_CLK]

#set_property PACKAGE_PIN D16 [get_ports {LED[0]}]
#set_property PACKAGE_PIN G16 [get_ports {LED[1]}]
#set_property PACKAGE_PIN H16 [get_ports {LED[2]}]
#set_property PACKAGE_PIN E18 [get_ports {LED[3]}]
#set_property PACKAGE_PIN E17 [get_ports {LED[4]}]
#set_property PACKAGE_PIN E16 [get_ports {LED[5]}]
#set_property PACKAGE_PIN H18 [get_ports {LED[6]}]
#set_property PACKAGE_PIN H17 [get_ports {LED[7]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {LED[*]}]

# bank 64 - FMC
set_property IOSTANDARD LVCMOS18 [get_ports -of_objects [get_iobanks 64]]
# bank 65 - FMC
set_property IOSTANDARD LVCMOS18 [get_ports -of_objects [get_iobanks 65]]

set_property PACKAGE_PIN F8 [get_ports IR_OUT]; # PMOD2_1
set_property IOSTANDARD LVCMOS18 [get_ports IR_OUT]
set_property PACKAGE_PIN G12 [get_ports EXT_MCU_SCL]; # PMOD2_6
set_property IOSTANDARD LVCMOS18 [get_ports EXT_MCU_SCL]
set_property PACKAGE_PIN F12 [get_ports EXT_MCU_SDA]; # PMOD2_7
set_property IOSTANDARD LVCMOS18 [get_ports EXT_MCU_SDA]

# external MCU SPI
set_property PACKAGE_PIN E8  [get_ports {EXT_MCU_DOUT[0]}]; # PMOD2_2
set_property PACKAGE_PIN D8  [get_ports {EXT_MCU_SCLK[0]}]; # PMOD2_3
#set_property PACKAGE_PIN E10 [get_ports {EXT_MCU_DOUT[]}]; # PMOD2_4
#set_property PACKAGE_PIN D10 [get_ports {EXT_MCU_SCLK[]}]; # PMOD2_5
#set_property PACKAGE_PIN G10 [get_ports {EXT_MCU_DOUT[]}]; # PMOD2_6
#set_property PACKAGE_PIN F12 [get_ports {EXT_MCU_SCLK[]}]; # PMOD2_7
set_property PACKAGE_PIN J13 [get_ports {EXT_MCU_DOUT[1]}]; # PMOD1_0
set_property PACKAGE_PIN H13 [get_ports {EXT_MCU_SCLK[1]}]; # PMOD1_1
set_property PACKAGE_PIN A13 [get_ports {EXT_MCU_DOUT[2]}]; # PMOD1_2
set_property PACKAGE_PIN A12 [get_ports {EXT_MCU_SCLK[2]}]; # PMOD1_3
set_property PACKAGE_PIN C12 [get_ports {EXT_MCU_DOUT[3]}]; # PMOD1_4
set_property PACKAGE_PIN B12 [get_ports {EXT_MCU_SCLK[3]}]; # PMOD1_5
set_property PACKAGE_PIN D13 [get_ports {EXT_MCU_DOUT[4]}]; # PMOD1_6
set_property PACKAGE_PIN C13 [get_ports {EXT_MCU_SCLK[4]}]; # PMOD1_7
#set_property PACKAGE_PIN F18 [get_ports {EXT_MCU_DOUT[1]}]; # PMOD3_0
#set_property PACKAGE_PIN F19 [get_ports {EXT_MCU_SCLK[1]}]; # PMOD3_1
#set_property PACKAGE_PIN G17 [get_ports {EXT_MCU_DOUT[2]}]; # PMOD3_2
#set_property PACKAGE_PIN F17 [get_ports {EXT_MCU_SCLK[2]}]; # PMOD3_3
#set_property PACKAGE_PIN G19 [get_ports {EXT_MCU_DOUT[3]}]; # PMOD3_4
#set_property PACKAGE_PIN F20 [get_ports {EXT_MCU_SCLK[3]}]; # PMOD3_5
#set_property PACKAGE_PIN C17 [get_ports {EXT_MCU_DOUT[4]}]; # PMOD3_6
#set_property PACKAGE_PIN B17 [get_ports {EXT_MCU_SCLK[4]}]; # PMOD3_7
set_property IOSTANDARD LVCMOS18 [get_ports {EXT_MCU_DOUT[*]}]
set_property IOSTANDARD LVCMOS18 [get_ports {EXT_MCU_SCLK[*]}]

set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]
set_property BITSTREAM.CONFIG.CONFIGRATE 50 [current_design]
set_property BITSTREAM.CONFIG.SPI_BUSWIDTH 4 [current_design]

