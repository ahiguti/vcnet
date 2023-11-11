
// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module ftdi_stream(
FTDI_CLK, FTDI_CLK_DO, FTDI_RESET_N, FTDI_DATA, FTDI_BE, FTDI_RXF_N,
FTDI_TXE_N, FTDI_WR_N, FTDI_RD_N, FTDI_OE_N, FTDI_SIWU_N, DEBUG_SW, DEBUG_LD,
SOUT_RDY, SOUT_VLD, SOUT_DATA, SIN_RDY, SIN_VLD,SIN_DATA, READTR_READY
);

parameter DATA_BITS = 32;
parameter BE_BITS = 4;
parameter FF_OUT_ADDR_BITS = 4;
parameter FF_IN_ADDR_BITS = 4;
parameter ENABLE_DEBUG_OUT = 0;

input FTDI_CLK;
input FTDI_CLK_DO;
input FTDI_RESET_N;
inout [DATA_BITS-1:0] FTDI_DATA;
inout [BE_BITS-1:0] FTDI_BE;
input FTDI_RXF_N;
input FTDI_TXE_N;
output FTDI_WR_N;
output FTDI_RD_N;
output FTDI_OE_N;
output FTDI_SIWU_N;
input [7:0] DEBUG_SW;
output [7:0] DEBUG_LD;
input SOUT_RDY;
output SOUT_VLD;
output [DATA_BITS-1:0] SOUT_DATA;
output READTR_READY;
output SIN_RDY;
input SIN_VLD;
input [DATA_BITS-1:0] SIN_DATA;

wire tostr_cmd_push;
wire tostr_cmd_peek;
wire tostr_cmd_pop;
wire tostr_cmd_prst;
wire [DATA_BITS-1:0] tostr_wdata;
wire [DATA_BITS-1:0] tostr_rdata;
wire [FF_OUT_ADDR_BITS:0] tostr_cur_pushlen;
wire [FF_OUT_ADDR_BITS:0] tostr_cur_peeklen;
wire [FF_OUT_ADDR_BITS:0] tostr_cur_poplen;
wire tostr_cur_nopush;
wire tostr_cur_nopeek;
wire tostr_cur_nopop;

wire fromstr_cmd_push;
wire fromstr_cmd_peek;
wire fromstr_cmd_pop;
wire fromstr_cmd_prst;
wire [DATA_BITS-1:0] fromstr_wdata;
wire [DATA_BITS-1:0] fromstr_rdata;
wire [FF_IN_ADDR_BITS:0] fromstr_cur_pushlen;
wire [FF_IN_ADDR_BITS:0] fromstr_cur_peeklen;
wire [FF_IN_ADDR_BITS:0] fromstr_cur_poplen;
wire fromstr_cur_nopush;
wire fromstr_cur_nopeek;
wire fromstr_cur_nopop;

ftdi_fifo
  #(.DATA_BITS(DATA_BITS), .BE_BITS(BE_BITS),
    .FF_OUT_ADDR_BITS(FF_OUT_ADDR_BITS), .FF_IN_ADDR_BITS(FF_IN_ADDR_BITS),
    .ENABLE_DEBUG_OUT(ENABLE_DEBUG_OUT))
ffifo(
    .FTDI_CLK(FTDI_CLK), .FTDI_CLK_DO(FTDI_CLK_DO),
    .FTDI_RESET_N(FTDI_RESET_N), .FTDI_DATA(FTDI_DATA), .FTDI_BE(FTDI_BE),
    .FTDI_RXF_N(FTDI_RXF_N), .FTDI_TXE_N(FTDI_TXE_N), .FTDI_WR_N(FTDI_WR_N),
    .FTDI_RD_N(FTDI_RD_N), .FTDI_OE_N(FTDI_OE_N), .FTDI_SIWU_N(FTDI_SIWU_N),
    .FF_CMD_PUSH(tostr_cmd_push), .FF_CMD_PEEK(fromstr_cmd_peek),
    .FF_CMD_POP(fromstr_cmd_pop), .FF_CMD_PRST(fromstr_cmd_prst),
    .FF_WDATA(tostr_wdata), .FF_RDATA(fromstr_rdata),
    .FF_CUR_PUSHLEN(tostr_cur_pushlen), .FF_CUR_PEEKLEN(fromstr_cur_peeklen),
    .FF_CUR_POPLEN(fromstr_cur_poplen), .FF_CUR_NOPUSH(tostr_cur_nopush),
    .FF_CUR_NOPEEK(fromstr_cur_nopeek), .FF_CUR_NOPOP(fromstr_cur_nopop),
    .FF_READTR_READY(READTR_READY), .DEBUG_SW(DEBUG_SW), .DEBUG_LD(DEBUG_LD));

fifo_reg
  #(.ADDR_BITS(FF_OUT_ADDR_BITS), .DATA_BITS(DATA_BITS))
fifo_tostr(
    .CLK(FTDI_CLK), .RESET_N(FTDI_RESET_N), .CMD_PUSH(tostr_cmd_push),
    .CMD_PEEK(tostr_cmd_peek), .CMD_POP(tostr_cmd_pop),
    .CMD_PRST(tostr_cmd_prst), .WDATA(tostr_wdata), .RDATA(tostr_rdata),
    .CUR_PUSHLEN(tostr_cur_pushlen), .CUR_PEEKLEN(tostr_cur_peeklen),
    .CUR_POPLEN(tostr_cur_poplen), .CUR_NOPUSH(tostr_cur_nopush),
    .CUR_NOPEEK(tostr_cur_nopeek), .CUR_NOPOP(tostr_cur_nopop));

fifo_reg
  #(.ADDR_BITS(FF_IN_ADDR_BITS), .DATA_BITS(DATA_BITS))
fifo_fromstr(
    .CLK(FTDI_CLK), .RESET_N(FTDI_RESET_N), .CMD_PUSH(fromstr_cmd_push),
    .CMD_PEEK(fromstr_cmd_peek), .CMD_POP(fromstr_cmd_pop),
    .CMD_PRST(fromstr_cmd_prst), .WDATA(fromstr_wdata), .RDATA(fromstr_rdata),
    .CUR_PUSHLEN(fromstr_cur_pushlen), .CUR_PEEKLEN(fromstr_cur_peeklen),
    .CUR_POPLEN(fromstr_cur_poplen), .CUR_NOPUSH(fromstr_cur_nopush),
    .CUR_NOPEEK(fromstr_cur_nopeek), .CUR_NOPOP(fromstr_cur_nopop));

fifo_stream
  #(.FF_ADDR_BITS(FF_OUT_ADDR_BITS), .DATA_BITS(DATA_BITS))
outstr(
    .CLK(FTDI_CLK), .RESET_N(FTDI_RESET_N), .FF_CMD_PEEK(tostr_cmd_peek),
    .FF_CMD_POP(tostr_cmd_pop), .FF_CMD_PRST(tostr_cmd_prst),
    .FF_RDATA(tostr_rdata), .FF_PEEKLEN(tostr_cur_peeklen),
    .FF_POPLEN(tostr_cur_poplen), .FF_NOPEEK(tostr_cur_nopeek), .FF_NOPOP(tostr_cur_nopop),
    .SOUT_RDY(SOUT_RDY), .SOUT_VLD(SOUT_VLD), .SOUT_DATA(SOUT_DATA));

stream_fifo
  #(.FF_ADDR_BITS(FF_IN_ADDR_BITS), .DATA_BITS(DATA_BITS))
instr(
    .SIN_RDY(SIN_RDY), .SIN_VLD(SIN_VLD), .SIN_DATA(SIN_DATA),
    .FF_CMD_PUSH(fromstr_cmd_push), .FF_WDATA(fromstr_wdata),
    .FF_NOPUSH(fromstr_cur_nopush), .FF_PUSHLEN(fromstr_cur_pushlen));

endmodule

