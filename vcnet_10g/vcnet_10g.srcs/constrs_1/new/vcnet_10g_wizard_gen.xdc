create_clock -period 10.000 -name VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0 -waveform {0.000 5.000}
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay 1.000 [get_ports {EXT_MCU_SCLK[*]}]
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 2.000 [get_ports {EXT_MCU_SCLK[*]}]
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay 1.000 [get_ports EXT_MCU_SCL]
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 2.000 [get_ports EXT_MCU_SCL]
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay 1.000 [get_ports EXT_MCU_SDA]
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 2.000 [get_ports EXT_MCU_SDA]
set_input_delay -clock [get_clocks MGTCLK0_clk_p] -min -add_delay 1.000 [get_ports HDMI1_AP]
set_input_delay -clock [get_clocks MGTCLK0_clk_p] -max -add_delay 2.000 [get_ports HDMI1_AP]
set_input_delay -clock [get_clocks MGTCLK0_clk_p] -min -add_delay 1.000 [get_ports HDMI1_LRCLK]
set_input_delay -clock [get_clocks MGTCLK0_clk_p] -max -add_delay 2.000 [get_ports HDMI1_LRCLK]
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay 1.000 [get_ports HDMI1_SCL]
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 2.000 [get_ports HDMI1_SCL]
set_input_delay -clock [get_clocks MGTCLK0_clk_p] -min -add_delay 1.000 [get_ports HDMI1_SCLK]
set_input_delay -clock [get_clocks MGTCLK0_clk_p] -max -add_delay 2.000 [get_ports HDMI1_SCLK]
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay 1.000 [get_ports HDMI1_SDA]
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 2.000 [get_ports HDMI1_SDA]
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay 1.000 [get_ports HDMI_SCL]
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 2.000 [get_ports HDMI_SCL]
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay 1.000 [get_ports HDMI_SDA]
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 2.000 [get_ports HDMI_SDA]
set_input_delay -clock [get_clocks MGTCLK0_clk_p] -min -add_delay 1.000 [get_ports SFP1_RX_LOS]
set_input_delay -clock [get_clocks MGTCLK0_clk_p] -max -add_delay 2.000 [get_ports SFP1_RX_LOS]
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay 1.000 [get_ports SFP1_RX_LOS]
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 2.000 [get_ports SFP1_RX_LOS]
set_input_delay -clock [get_clocks MGTCLK0_clk_p] -min -add_delay 1.000 [get_ports reset]
set_input_delay -clock [get_clocks MGTCLK0_clk_p] -max -add_delay 2.000 [get_ports reset]
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay 1.000 [get_ports reset]
set_input_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 2.000 [get_ports reset]
set_input_delay -clock [get_clocks HDMI1_LLC] -min -add_delay 1.000 [get_ports reset]
set_input_delay -clock [get_clocks HDMI1_LLC] -max -add_delay 2.000 [get_ports reset]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay -1.000 [get_ports {EXT_MCU_DOUT[*]}]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 1.000 [get_ports {EXT_MCU_DOUT[*]}]
set_output_delay -clock [get_clocks MGTCLK0_clk_p] -clock_fall -min -add_delay -1.000 [get_ports {{LED[1]} {LED[2]}}]
set_output_delay -clock [get_clocks MGTCLK0_clk_p] -clock_fall -max -add_delay 1.000 [get_ports {{LED[1]} {LED[2]}}]
set_output_delay -clock [get_clocks MGTCLK0_clk_p] -min -add_delay -1.000 [get_ports {{LED[1]} {LED[2]}}]
set_output_delay -clock [get_clocks MGTCLK0_clk_p] -max -add_delay 1.000 [get_ports {{LED[1]} {LED[2]}}]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay -1.000 [get_ports {{LED[2]} {LED[0]} {LED[1]}}]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 1.000 [get_ports {{LED[2]} {LED[0]} {LED[1]}}]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay -1.000 [get_ports EXT_MCU_SCL]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 1.000 [get_ports EXT_MCU_SCL]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay -1.000 [get_ports EXT_MCU_SDA]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 1.000 [get_ports EXT_MCU_SDA]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay -1.000 [get_ports HDMI1_SCL]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 1.000 [get_ports HDMI1_SCL]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay -1.000 [get_ports HDMI1_SDA]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 1.000 [get_ports HDMI1_SDA]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay -1.000 [get_ports HDMI_SCL]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 1.000 [get_ports HDMI_SCL]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay -1.000 [get_ports HDMI_SDA]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 1.000 [get_ports HDMI_SDA]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -min -add_delay -1.000 [get_ports IR_OUT]
set_output_delay -clock [get_clocks VIRTUAL_clk_100_bd_vcnet_10g_clk_wiz_sys_0] -max -add_delay 1.000 [get_ports IR_OUT]
set_output_delay -clock [get_clocks MGTCLK0_clk_p] -min -add_delay -1.000 [get_ports SFP1_TX_DISABLE]
set_output_delay -clock [get_clocks MGTCLK0_clk_p] -max -add_delay 1.000 [get_ports SFP1_TX_DISABLE]
