
// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module ftdi_axim(
FTDI_CLK, FTDI_CLK_DO, FTDI_RESET_N, FTDI_DATA, FTDI_BE, FTDI_RXF_N,
FTDI_TXE_N, FTDI_WR_N, FTDI_RD_N, FTDI_OE_N, FTDI_SIWU_N, DEBUG_SW, DEBUG_LD,
ARADDR, ARVALID, ARREADY, ARID, ARLEN, ARSIZE, ARBURST, ARLOCK, ARCACHE,
ARPROT, ARQOS, RID, RDATA, RRESP, RLAST, RUSER, RVALID, RREADY, AWADDR,
AWVALID, AWREADY, AWID, AWLEN, AWSIZE, AWBURST, AWLOCK, AWCACHE, AWPROT,
AWQOS, AWUSER, WID, WDATA, WSTRB, WLAST, WVALID, WREADY, WUSER, BID, BRESP,
BVALID, BREADY, BUSER
);
parameter DATA_BITS = 32;
parameter BE_BITS = 4;
parameter FF_TOAXI_ADDR_BITS = 4;
parameter FF_FROMAXI_ADDR_BITS = 4;
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

output [31:0] ARADDR;
output ARVALID;
input ARREADY;
output [0:0] ARID;
output [7:0] ARLEN;
output [2:0] ARSIZE;
output [1:0] ARBURST;
output [1:0] ARLOCK;
output [3:0] ARCACHE;
output [2:0] ARPROT;
output [3:0] ARQOS;
input [0:0] RID;
input [31:0] RDATA;
input [1:0] RRESP;
input RLAST;
input [0:0] RUSER;
input RVALID;
output RREADY;
output [31:0] AWADDR;
output AWVALID;
input AWREADY;
output [0:0] AWID;
output [7:0] AWLEN;
output [2:0] AWSIZE;
output [1:0] AWBURST;
output [1:0] AWLOCK;
output [3:0] AWCACHE;
output [2:0] AWPROT;
output [3:0] AWQOS;
output [0:0] AWUSER;
output [0:0] WID;
output [31:0] WDATA;
output [7:0] WSTRB;
output WLAST;
output WVALID;
input WREADY;
output [0:0] WUSER;
input [0:0] BID;
input [1:0] BRESP;
input BVALID;
output BREADY;
input [0:0] BUSER;

wire toaxi_rdy;
wire toaxi_vld;
wire [DATA_BITS-1:0] toaxi_data;
wire readtr_ready;
wire fromaxi_rdy;
wire fromaxi_vld;
wire [DATA_BITS-1:0] fromaxi_data;

ftdi_stream
    #(.DATA_BITS(DATA_BITS), .BE_BITS(BE_BITS),
      .FF_OUT_ADDR_BITS(FF_TOAXI_ADDR_BITS),
      .FF_IN_ADDR_BITS(FF_FROMAXI_ADDR_BITS),
      .ENABLE_DEBUG_OUT(ENABLE_DEBUG_OUT))
inst_ftdi_stream(
    .FTDI_CLK(FTDI_CLK), .FTDI_CLK_DO(FTDI_CLK_DO), .FTDI_RESET_N(FTDI_RESET_N),
    .FTDI_DATA(FTDI_DATA), .FTDI_BE(FTDI_BE), .FTDI_RXF_N(FTDI_RXF_N),
    .FTDI_TXE_N(FTDI_TXE_N), .FTDI_WR_N(FTDI_WR_N), .FTDI_RD_N(FTDI_RD_N),
    .FTDI_OE_N(FTDI_OE_N), .FTDI_SIWU_N(FTDI_SIWU_N), .DEBUG_SW(DEBUG_SW),
    .DEBUG_LD(DEBUG_LD), .SOUT_RDY(toaxi_rdy), .SOUT_VLD(toaxi_vld),
    .SOUT_DATA(toaxi_data), .READTR_READY(readtr_ready),
    .SIN_RDY(fromaxi_rdy), .SIN_VLD(fromaxi_vld),
    .SIN_DATA(fromaxi_data));

stream_axim
  #(.DATA_BITS(DATA_BITS))
inst_stream_axim(
    .CLK(FTDI_CLK), .RESET_N(FTDI_RESET_N), .TOAXI_VLD(toaxi_vld),
    .TOAXI_RDY(toaxi_rdy), .TOAXI_DATA(toaxi_data),
    .READTR_READY(readtr_ready), .FROMAXI_VLD(fromaxi_vld),
    .FROMAXI_RDY(fromaxi_rdy), .FROMAXI_DATA(fromaxi_data),
    .ARADDR(ARADDR), .ARVALID(ARVALID), .ARREADY(ARREADY), .ARID(ARID),
    .ARLEN(ARLEN), .ARSIZE(ARSIZE), .ARBURST(ARBURST), .ARLOCK(ARLOCK),
    .ARCACHE(ARCACHE), .ARPROT(ARPROT), .ARQOS(ARQOS), .RID(RID),
    .RDATA(RDATA), .RRESP(RRESP), .RLAST(RLAST), .RUSER(RUSER),
    .RVALID(RVALID), .RREADY(RREADY), .AWADDR(AWADDR), .AWVALID(AWVALID),
    .AWREADY(AWREADY), .AWID(AWID), .AWLEN(AWLEN), .AWSIZE(AWSIZE),
    .AWBURST(AWBURST), .AWLOCK(AWLOCK), .AWCACHE(AWCACHE), .AWPROT(AWPROT),
    .AWQOS(AWQOS), .AWUSER(AWUSER), .WID(WID), .WDATA(WDATA), .WSTRB(WSTRB),
    .WLAST(WLAST), .WVALID(WVALID), .WREADY(WREADY), .WUSER(WUSER),
    .BID(BID), .BRESP(BRESP), .BVALID(BVALID), .BREADY(BREADY),
    .BUSER(BUSER));

endmodule
