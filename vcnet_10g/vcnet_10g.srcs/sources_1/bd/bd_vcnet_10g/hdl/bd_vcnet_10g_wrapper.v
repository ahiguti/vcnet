//Copyright 1986-2018 Xilinx, Inc. All Rights Reserved.
//--------------------------------------------------------------------------------
//Tool Version: Vivado v.2018.2 (lin64) Build 2258646 Thu Jun 14 20:02:38 MDT 2018
//Date        : Sat Dec 31 06:51:55 2022
//Host        : habuild running 64-bit Ubuntu 22.04.1 LTS
//Command     : generate_target bd_vcnet_10g_wrapper.bd
//Design      : bd_vcnet_10g_wrapper
//Purpose     : IP block netlist
//--------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

module bd_vcnet_10g_wrapper
   (EXT_MCU_DOUT,
    EXT_MCU_SCL,
    EXT_MCU_SCLK,
    EXT_MCU_SDA,
    HDMI1_AP,
    HDMI1_DE,
    HDMI1_HS,
    HDMI1_LLC,
    HDMI1_LRCLK,
    HDMI1_P,
    HDMI1_SCL,
    HDMI1_SCLK,
    HDMI1_SDA,
    HDMI1_VS,
    IR_OUT,
    LED,
    MGTCLK0_clk_n,
    MGTCLK0_clk_p,
    SFP1_RX_LOS,
    SFP1_RX_N,
    SFP1_RX_P,
    SFP1_TX_DISABLE,
    SFP1_TX_N,
    SFP1_TX_P,
    default_sysclk_250_clk_n,
    default_sysclk_250_clk_p,
    reset);
  output [4:0]EXT_MCU_DOUT;
  inout EXT_MCU_SCL;
  input [4:0]EXT_MCU_SCLK;
  inout EXT_MCU_SDA;
  input HDMI1_AP;
  input HDMI1_DE;
  input HDMI1_HS;
  input HDMI1_LLC;
  input HDMI1_LRCLK;
  input [23:0]HDMI1_P;
  inout HDMI1_SCL;
  input HDMI1_SCLK;
  inout HDMI1_SDA;
  input HDMI1_VS;
  output IR_OUT;
  output [2:0]LED;
  input MGTCLK0_clk_n;
  input MGTCLK0_clk_p;
  input SFP1_RX_LOS;
  input SFP1_RX_N;
  input SFP1_RX_P;
  output SFP1_TX_DISABLE;
  output SFP1_TX_N;
  output SFP1_TX_P;
  input default_sysclk_250_clk_n;
  input default_sysclk_250_clk_p;
  input reset;

  wire [4:0]EXT_MCU_DOUT;
  wire EXT_MCU_SCL;
  wire [4:0]EXT_MCU_SCLK;
  wire EXT_MCU_SDA;
  wire HDMI1_AP;
  wire HDMI1_DE;
  wire HDMI1_HS;
  wire HDMI1_LLC;
  wire HDMI1_LRCLK;
  wire [23:0]HDMI1_P;
  wire HDMI1_SCL;
  wire HDMI1_SCLK;
  wire HDMI1_SDA;
  wire HDMI1_VS;
  wire IR_OUT;
  wire [2:0]LED;
  wire MGTCLK0_clk_n;
  wire MGTCLK0_clk_p;
  wire SFP1_RX_LOS;
  wire SFP1_RX_N;
  wire SFP1_RX_P;
  wire SFP1_TX_DISABLE;
  wire SFP1_TX_N;
  wire SFP1_TX_P;
  wire default_sysclk_250_clk_n;
  wire default_sysclk_250_clk_p;
  wire reset;

  bd_vcnet_10g bd_vcnet_10g_i
       (.EXT_MCU_DOUT(EXT_MCU_DOUT),
        .EXT_MCU_SCL(EXT_MCU_SCL),
        .EXT_MCU_SCLK(EXT_MCU_SCLK),
        .EXT_MCU_SDA(EXT_MCU_SDA),
        .HDMI1_AP(HDMI1_AP),
        .HDMI1_DE(HDMI1_DE),
        .HDMI1_HS(HDMI1_HS),
        .HDMI1_LLC(HDMI1_LLC),
        .HDMI1_LRCLK(HDMI1_LRCLK),
        .HDMI1_P(HDMI1_P),
        .HDMI1_SCL(HDMI1_SCL),
        .HDMI1_SCLK(HDMI1_SCLK),
        .HDMI1_SDA(HDMI1_SDA),
        .HDMI1_VS(HDMI1_VS),
        .IR_OUT(IR_OUT),
        .LED(LED),
        .MGTCLK0_clk_n(MGTCLK0_clk_n),
        .MGTCLK0_clk_p(MGTCLK0_clk_p),
        .SFP1_RX_LOS(SFP1_RX_LOS),
        .SFP1_RX_N(SFP1_RX_N),
        .SFP1_RX_P(SFP1_RX_P),
        .SFP1_TX_DISABLE(SFP1_TX_DISABLE),
        .SFP1_TX_N(SFP1_TX_N),
        .SFP1_TX_P(SFP1_TX_P),
        .default_sysclk_250_clk_n(default_sysclk_250_clk_n),
        .default_sysclk_250_clk_p(default_sysclk_250_clk_p),
        .reset(reset));
endmodule
