//Copyright 1986-2021 Xilinx, Inc. All Rights Reserved.
//--------------------------------------------------------------------------------
//Tool Version: Vivado v.2021.2 (win64) Build 3367213 Tue Oct 19 02:48:09 MDT 2021
//Date        : Sun Jan  1 21:21:41 2023
//Host        : ha117 running 64-bit major release  (build 9200)
//Command     : generate_target bd_vcnet_1g_wrapper.bd
//Design      : bd_vcnet_1g_wrapper
//Purpose     : IP block netlist
//--------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

module bd_vcnet_1g_wrapper
   (DDC_scl_io,
    DDC_sda_io,
    DDR_addr,
    DDR_ba,
    DDR_cas_n,
    DDR_ck_n,
    DDR_ck_p,
    DDR_cke,
    DDR_cs_n,
    DDR_dm,
    DDR_dq,
    DDR_dqs_n,
    DDR_dqs_p,
    DDR_odt,
    DDR_ras_n,
    DDR_reset_n,
    DDR_we_n,
    EXT_MCU_DOUT,
    EXT_MCU_SCLK,
    FIXED_IO_ddr_vrn,
    FIXED_IO_ddr_vrp,
    FIXED_IO_mio,
    FIXED_IO_ps_clk,
    FIXED_IO_ps_porb,
    FIXED_IO_ps_srstb,
    I2C_SCL_0,
    I2C_SDA_0,
    I2S_BCLK_0,
    I2S_DAT_0,
    I2S_LR_0,
    IR_OUT_0,
    LED_0,
    TMDS_1_clk_n,
    TMDS_1_clk_p,
    TMDS_1_data_n,
    TMDS_1_data_p,
    TMDS_clk_n,
    TMDS_clk_p,
    TMDS_data_n,
    TMDS_data_p,
    hdmi_hpd_tri_o);
  inout DDC_scl_io;
  inout DDC_sda_io;
  inout [14:0]DDR_addr;
  inout [2:0]DDR_ba;
  inout DDR_cas_n;
  inout DDR_ck_n;
  inout DDR_ck_p;
  inout DDR_cke;
  inout DDR_cs_n;
  inout [3:0]DDR_dm;
  inout [31:0]DDR_dq;
  inout [3:0]DDR_dqs_n;
  inout [3:0]DDR_dqs_p;
  inout DDR_odt;
  inout DDR_ras_n;
  inout DDR_reset_n;
  inout DDR_we_n;
  output [4:0]EXT_MCU_DOUT;
  input [4:0]EXT_MCU_SCLK;
  inout FIXED_IO_ddr_vrn;
  inout FIXED_IO_ddr_vrp;
  inout [53:0]FIXED_IO_mio;
  inout FIXED_IO_ps_clk;
  inout FIXED_IO_ps_porb;
  inout FIXED_IO_ps_srstb;
  inout I2C_SCL_0;
  inout I2C_SDA_0;
  input I2S_BCLK_0;
  input I2S_DAT_0;
  input I2S_LR_0;
  output IR_OUT_0;
  output [3:0]LED_0;
  output TMDS_1_clk_n;
  output TMDS_1_clk_p;
  output [2:0]TMDS_1_data_n;
  output [2:0]TMDS_1_data_p;
  input TMDS_clk_n;
  input TMDS_clk_p;
  input [2:0]TMDS_data_n;
  input [2:0]TMDS_data_p;
  output [0:0]hdmi_hpd_tri_o;

  wire DDC_scl_i;
  wire DDC_scl_io;
  wire DDC_scl_o;
  wire DDC_scl_t;
  wire DDC_sda_i;
  wire DDC_sda_io;
  wire DDC_sda_o;
  wire DDC_sda_t;
  wire [14:0]DDR_addr;
  wire [2:0]DDR_ba;
  wire DDR_cas_n;
  wire DDR_ck_n;
  wire DDR_ck_p;
  wire DDR_cke;
  wire DDR_cs_n;
  wire [3:0]DDR_dm;
  wire [31:0]DDR_dq;
  wire [3:0]DDR_dqs_n;
  wire [3:0]DDR_dqs_p;
  wire DDR_odt;
  wire DDR_ras_n;
  wire DDR_reset_n;
  wire DDR_we_n;
  wire [4:0]EXT_MCU_DOUT;
  wire [4:0]EXT_MCU_SCLK;
  wire FIXED_IO_ddr_vrn;
  wire FIXED_IO_ddr_vrp;
  wire [53:0]FIXED_IO_mio;
  wire FIXED_IO_ps_clk;
  wire FIXED_IO_ps_porb;
  wire FIXED_IO_ps_srstb;
  wire I2C_SCL_0;
  wire I2C_SDA_0;
  wire I2S_BCLK_0;
  wire I2S_DAT_0;
  wire I2S_LR_0;
  wire IR_OUT_0;
  wire [3:0]LED_0;
  wire TMDS_1_clk_n;
  wire TMDS_1_clk_p;
  wire [2:0]TMDS_1_data_n;
  wire [2:0]TMDS_1_data_p;
  wire TMDS_clk_n;
  wire TMDS_clk_p;
  wire [2:0]TMDS_data_n;
  wire [2:0]TMDS_data_p;
  wire [0:0]hdmi_hpd_tri_o;

  IOBUF DDC_scl_iobuf
       (.I(DDC_scl_o),
        .IO(DDC_scl_io),
        .O(DDC_scl_i),
        .T(DDC_scl_t));
  IOBUF DDC_sda_iobuf
       (.I(DDC_sda_o),
        .IO(DDC_sda_io),
        .O(DDC_sda_i),
        .T(DDC_sda_t));
  bd_vcnet_1g bd_vcnet_1g_i
       (.DDC_scl_i(DDC_scl_i),
        .DDC_scl_o(DDC_scl_o),
        .DDC_scl_t(DDC_scl_t),
        .DDC_sda_i(DDC_sda_i),
        .DDC_sda_o(DDC_sda_o),
        .DDC_sda_t(DDC_sda_t),
        .DDR_addr(DDR_addr),
        .DDR_ba(DDR_ba),
        .DDR_cas_n(DDR_cas_n),
        .DDR_ck_n(DDR_ck_n),
        .DDR_ck_p(DDR_ck_p),
        .DDR_cke(DDR_cke),
        .DDR_cs_n(DDR_cs_n),
        .DDR_dm(DDR_dm),
        .DDR_dq(DDR_dq),
        .DDR_dqs_n(DDR_dqs_n),
        .DDR_dqs_p(DDR_dqs_p),
        .DDR_odt(DDR_odt),
        .DDR_ras_n(DDR_ras_n),
        .DDR_reset_n(DDR_reset_n),
        .DDR_we_n(DDR_we_n),
        .EXT_MCU_DOUT(EXT_MCU_DOUT),
        .EXT_MCU_SCLK(EXT_MCU_SCLK),
        .FIXED_IO_ddr_vrn(FIXED_IO_ddr_vrn),
        .FIXED_IO_ddr_vrp(FIXED_IO_ddr_vrp),
        .FIXED_IO_mio(FIXED_IO_mio),
        .FIXED_IO_ps_clk(FIXED_IO_ps_clk),
        .FIXED_IO_ps_porb(FIXED_IO_ps_porb),
        .FIXED_IO_ps_srstb(FIXED_IO_ps_srstb),
        .I2C_SCL_0(I2C_SCL_0),
        .I2C_SDA_0(I2C_SDA_0),
        .I2S_BCLK_0(I2S_BCLK_0),
        .I2S_DAT_0(I2S_DAT_0),
        .I2S_LR_0(I2S_LR_0),
        .IR_OUT_0(IR_OUT_0),
        .LED_0(LED_0),
        .TMDS_1_clk_n(TMDS_1_clk_n),
        .TMDS_1_clk_p(TMDS_1_clk_p),
        .TMDS_1_data_n(TMDS_1_data_n),
        .TMDS_1_data_p(TMDS_1_data_p),
        .TMDS_clk_n(TMDS_clk_n),
        .TMDS_clk_p(TMDS_clk_p),
        .TMDS_data_n(TMDS_data_n),
        .TMDS_data_p(TMDS_data_p),
        .hdmi_hpd_tri_o(hdmi_hpd_tri_o));
endmodule
