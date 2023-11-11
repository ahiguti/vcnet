
set_false_path -to [get_cells -hier {*metastability_guard*}]
set_false_path -to [get_ports {LED[*]}]
set_false_path -to [get_ports {SFP1_TX_DISABLE}]

#################################
set_property PACKAGE_PIN AD26 [get_ports SFP1_RX_LOS]
set_property PACKAGE_PIN AF24 [get_ports SFP1_TX_DISABLE]

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

#### AESKU040 ADV7511
set_property PACKAGE_PIN D21 [get_ports HDMI_SCL]
set_property IOSTANDARD LVCMOS18 [get_ports HDMI_SCL]
set_property SLEW SLOW [get_ports HDMI_SCL]
set_property DRIVE 8 [get_ports HDMI_SCL]
set_property PACKAGE_PIN E22 [get_ports HDMI_SDA]
set_property IOSTANDARD LVCMOS18 [get_ports HDMI_SDA]
set_property SLEW SLOW [get_ports HDMI_SDA]
set_property DRIVE 8 [get_ports HDMI_SDA]
#set_property PACKAGE_PIN E21 [get_ports HDMI_INT]
#set_property IOSTANDARD LVCMOS18 [get_ports HDMI_INT]
set_property PACKAGE_PIN B21 [get_ports HDMI_CLK]
set_property IOSTANDARD LVCMOS18 [get_ports HDMI_CLK]
set_property PACKAGE_PIN B22 [get_ports HDMI_DE]
set_property IOSTANDARD LVCMOS18 [get_ports HDMI_DE]
set_property PACKAGE_PIN C21 [get_ports HDMI_HSYNC]
set_property IOSTANDARD LVCMOS18 [get_ports HDMI_HSYNC]
set_property PACKAGE_PIN C22 [get_ports HDMI_VSYNC]
set_property IOSTANDARD LVCMOS18 [get_ports HDMI_VSYNC]
set_property PACKAGE_PIN A20 [get_ports {HDMI_D[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI_D[0]}]
set_property PACKAGE_PIN A22 [get_ports {HDMI_D[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI_D[1]}]
set_property PACKAGE_PIN A23 [get_ports {HDMI_D[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI_D[2]}]
set_property PACKAGE_PIN C26 [get_ports {HDMI_D[3]}]
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI_D[3]}]
set_property PACKAGE_PIN B26 [get_ports {HDMI_D[4]}]
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI_D[4]}]
set_property PACKAGE_PIN D24 [get_ports {HDMI_D[5]}]
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI_D[5]}]
set_property PACKAGE_PIN C24 [get_ports {HDMI_D[6]}]
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI_D[6]}]
set_property PACKAGE_PIN D23 [get_ports {HDMI_D[7]}]
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI_D[7]}]
set_property PACKAGE_PIN C23 [get_ports {HDMI_D[8]}]
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI_D[8]}]
set_property PACKAGE_PIN B24 [get_ports {HDMI_D[9]}]
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI_D[9]}]
set_property PACKAGE_PIN B25 [get_ports {HDMI_D[10]}]
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI_D[10]}]
set_property PACKAGE_PIN D25 [get_ports {HDMI_D[11]}]
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI_D[11]}]
set_property PACKAGE_PIN D26 [get_ports {HDMI_D[12]}]
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI_D[12]}]
set_property PACKAGE_PIN E23 [get_ports {HDMI_D[13]}]
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI_D[13]}]
set_property PACKAGE_PIN A24 [get_ports {HDMI_D[14]}]
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI_D[14]}]
set_property PACKAGE_PIN A25 [get_ports {HDMI_D[15]}]
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI_D[15]}]


# Digilent FMC-HDMI ADV7611
set_property PACKAGE_PIN AD20 [get_ports {HDMI1_P[0]}]
set_property PACKAGE_PIN AC21 [get_ports {HDMI1_P[1]}]
set_property PACKAGE_PIN AB21 [get_ports {HDMI1_P[2]}]
set_property PACKAGE_PIN AF13 [get_ports {HDMI1_P[3]}]
set_property PACKAGE_PIN AE15 [get_ports {HDMI1_P[4]}]
set_property PACKAGE_PIN AF14 [get_ports {HDMI1_P[5]}]
set_property PACKAGE_PIN AF15 [get_ports {HDMI1_P[6]}]
set_property PACKAGE_PIN AE16 [get_ports {HDMI1_P[7]}]
set_property PACKAGE_PIN AD13 [get_ports {HDMI1_P[8]}]
set_property PACKAGE_PIN AD15 [get_ports {HDMI1_P[9]}]
set_property PACKAGE_PIN AD14 [get_ports {HDMI1_P[10]}]
set_property PACKAGE_PIN AC13 [get_ports {HDMI1_P[11]}]
set_property PACKAGE_PIN AC14 [get_ports {HDMI1_P[12]}]
set_property PACKAGE_PIN W13 [get_ports {HDMI1_P[13]}]
set_property PACKAGE_PIN AB14 [get_ports {HDMI1_P[14]}]
set_property PACKAGE_PIN W14 [get_ports {HDMI1_P[15]}]
set_property PACKAGE_PIN AA14 [get_ports {HDMI1_P[16]}]
set_property PACKAGE_PIN AF10 [get_ports {HDMI1_P[17]}]
set_property PACKAGE_PIN AE10 [get_ports {HDMI1_P[18]}]
set_property PACKAGE_PIN AD8 [get_ports {HDMI1_P[19]}]
set_property PACKAGE_PIN AC8 [get_ports {HDMI1_P[20]}]
set_property PACKAGE_PIN AB9 [get_ports {HDMI1_P[21]}]
set_property PACKAGE_PIN AB10 [get_ports {HDMI1_P[22]}]
set_property PACKAGE_PIN AB11 [get_ports {HDMI1_P[23]}]
set_property IOSTANDARD LVCMOS18 [get_ports {HDMI1_P[*]}]
set_property PACKAGE_PIN AC19 [get_ports HDMI1_VS]
set_property PACKAGE_PIN AD21 [get_ports HDMI1_HS]
set_property PACKAGE_PIN AD19 [get_ports HDMI1_DE]
set_property PACKAGE_PIN AB12 [get_ports HDMI1_LLC]
set_property PACKAGE_PIN AB19 [get_ports HDMI1_SCLK]
set_property PACKAGE_PIN AE20 [get_ports HDMI1_LRCLK]
#set_property PACKAGE_PIN W20 [get_ports HDMI1_MCLK]; #LA17_P_CC HDMI1_MCLK
#set_property IOSTANDARD LVCMOS18 [get_ports HDMI1_MCLK];
set_property PACKAGE_PIN Y18 [get_ports HDMI1_AP]
set_property PACKAGE_PIN W18 [get_ports HDMI1_SDA]
set_property PACKAGE_PIN AE21 [get_ports HDMI1_SCL]
#set_property PACKAGE_PIN Y17 [get_ports HDMI1_RESETN]; #LA23_P HDMI1_RESETN
#set_property IOSTANDARD LVCMOS18 [get_ports HDMI1_RESETN];
#set_property PACKAGE_PIN W19 [get_ports HDMI1_INT1]; #LA25_N HDMI1_INT1
#set_property IOSTANDARD LVCMOS18 [get_ports HDMI1_INT1];

# bank 64 - FMC
set_property IOSTANDARD LVCMOS18 [get_ports -of_objects [get_iobanks 64]]
# bank 65 - FMC
set_property IOSTANDARD LVCMOS18 [get_ports -of_objects [get_iobanks 65]]

# InfraRed remote controller
set_property PACKAGE_PIN F8 [get_ports IR_OUT]
set_property IOSTANDARD LVCMOS18 [get_ports IR_OUT]

# external MCU I2S (unused)
set_property PACKAGE_PIN G12 [get_ports EXT_MCU_SCL]
set_property IOSTANDARD LVCMOS18 [get_ports EXT_MCU_SCL]
set_property PACKAGE_PIN F12 [get_ports EXT_MCU_SDA]
set_property IOSTANDARD LVCMOS18 [get_ports EXT_MCU_SDA]

# external MCU SPI
set_property PACKAGE_PIN E8 [get_ports {EXT_MCU_DOUT[0]}]
set_property PACKAGE_PIN D8 [get_ports {EXT_MCU_SCLK[0]}]
set_property PACKAGE_PIN J13 [get_ports {EXT_MCU_DOUT[1]}]
set_property PACKAGE_PIN H13 [get_ports {EXT_MCU_SCLK[1]}]
set_property PACKAGE_PIN A13 [get_ports {EXT_MCU_DOUT[2]}]
set_property PACKAGE_PIN A12 [get_ports {EXT_MCU_SCLK[2]}]
set_property PACKAGE_PIN C12 [get_ports {EXT_MCU_DOUT[3]}]
set_property PACKAGE_PIN B12 [get_ports {EXT_MCU_SCLK[3]}]
set_property PACKAGE_PIN D13 [get_ports {EXT_MCU_DOUT[4]}]
set_property PACKAGE_PIN C13 [get_ports {EXT_MCU_SCLK[4]}]
set_property IOSTANDARD LVCMOS18 [get_ports {EXT_MCU_DOUT[*]}]
set_property IOSTANDARD LVCMOS18 [get_ports {EXT_MCU_SCLK[*]}]


create_generated_clock -name HDMI_CLK_FWD -source [get_pins HDMI_CLK_OBUF_inst/O] -divide_by 1 [get_ports HDMI_CLK]
set_output_delay -clock HDMI_CLK_FWD -max 1.100 [get_ports HDMI_DE]
set_output_delay -clock HDMI_CLK_FWD -min -0.800 [get_ports HDMI_DE]
set_output_delay -clock HDMI_CLK_FWD -max 1.100 [get_ports HDMI_HSYNC]
set_output_delay -clock HDMI_CLK_FWD -min -0.800 [get_ports HDMI_HSYNC]
set_output_delay -clock HDMI_CLK_FWD -max 1.100 [get_ports HDMI_VSYNC]
set_output_delay -clock HDMI_CLK_FWD -min -0.800 [get_ports HDMI_VSYNC]
set_output_delay -clock HDMI_CLK_FWD -max 1.100 [get_ports {HDMI_D[*]}]
set_output_delay -clock HDMI_CLK_FWD -min -0.800 [get_ports {HDMI_D[*]}]

#create_clock -period 6.000 -name HDMI1_LLC [get_ports HDMI1_LLC]
set_input_delay -clock HDMI1_LLC -max 3.500 [get_ports {HDMI1_P[*]}]
set_input_delay -clock HDMI1_LLC -min 0.600 [get_ports {HDMI1_P[*]}]
set_input_delay -clock HDMI1_LLC -max 3.500 [get_ports HDMI1_VS]
set_input_delay -clock HDMI1_LLC -min 0.600 [get_ports HDMI1_VS]
set_input_delay -clock HDMI1_LLC -max 3.500 [get_ports HDMI1_HS]
set_input_delay -clock HDMI1_LLC -min 0.600 [get_ports HDMI1_HS]
set_input_delay -clock HDMI1_LLC -max 3.500 [get_ports HDMI1_DE]
set_input_delay -clock HDMI1_LLC -min 0.600 [get_ports HDMI1_DE]

# AESKU040 mt25qu256-spi-x1_x2_x4

set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]
set_property BITSTREAM.CONFIG.CONFIGRATE 50 [current_design]
set_property BITSTREAM.CONFIG.SPI_BUSWIDTH 4 [current_design]

