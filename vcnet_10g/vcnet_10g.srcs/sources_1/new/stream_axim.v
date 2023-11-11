
// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module stream_axim(
CLK, RESET_N, TOAXI_VLD, TOAXI_RDY, TOAXI_DATA, READTR_READY,
FROMAXI_VLD, FROMAXI_RDY, FROMAXI_DATA,
ARADDR, ARVALID, ARREADY, ARID, ARLEN, ARSIZE, ARBURST, ARLOCK, ARCACHE,
ARPROT, ARQOS, RID, RDATA, RRESP, RLAST, RUSER, RVALID, RREADY, AWADDR,
AWVALID, AWREADY, AWID, AWLEN, AWSIZE, AWBURST, AWLOCK, AWCACHE, AWPROT,
AWQOS, AWUSER, WID, WDATA, WSTRB, WLAST, WVALID, WREADY, WUSER, BID, BRESP,
BVALID, BREADY, BUSER
);
parameter DATA_BITS = 32;
input CLK;
input RESET_N;
input TOAXI_VLD;
output TOAXI_RDY;
input [DATA_BITS-1:0] TOAXI_DATA;
input READTR_READY;
output FROMAXI_VLD;
input FROMAXI_RDY;
output [DATA_BITS-1:0] FROMAXI_DATA;
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
input [DATA_BITS-1:0] RDATA;
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
output [DATA_BITS-1:0] WDATA;
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

wire rep_rdy;
wire rep_vld;
wire [31:0] rep_address;
wire [31:0] rep_count;
wire rep_mode_wr;
wire [1:0] rep_error;
wire ent_rrdy;
wire ent_rvld;
wire [DATA_BITS-1:0] ent_rdt;
wire ent_wrdy;
wire ent_wvld;
wire [DATA_BITS-1:0] ent_wdt;

stream_rep
  #(.DATA_BITS(DATA_BITS))
sre(
    .CLK(CLK), .RESET_N(RESET_N), .TOAXI_VLD(TOAXI_VLD),
    .TOAXI_RDY(TOAXI_RDY), .TOAXI_DATA(TOAXI_DATA),
    .READTR_READY(READTR_READY), .FROMAXI_VLD(FROMAXI_VLD),
    .FROMAXI_RDY(FROMAXI_RDY), .FROMAXI_DATA(FROMAXI_DATA),
    .REP_RDY(rep_rdy), .REP_VLD(rep_vld), .REP_ADDRESS(rep_address),
    .REP_COUNT(rep_count), .REP_MODE_WR(rep_mode_wr), .REP_ERROR(rep_error),
    .ENT_RRDY(ent_rrdy), .ENT_RVLD(ent_rvld), .ENT_RDT(ent_rdt),
    .ENT_WRDY(ent_wrdy), .ENT_WVLD(ent_wvld), .ENT_WDT(ent_wdt));

rep_aximaster
  #(.DATA_BITS(DATA_BITS))
reaxim(
    .CLK(CLK), .RESET_N(RESET_N), .ARADDR(ARADDR), .ARVALID(ARVALID),
    .ARREADY(ARREADY), .ARID(ARID), .ARLEN(ARLEN), .ARSIZE(ARSIZE),
    .ARBURST(ARBURST), .ARLOCK(ARLOCK), .ARCACHE(ARCACHE), .ARPROT(ARPROT),
    .ARQOS(ARQOS), .RID(RID), .RDATA(RDATA), .RRESP(RRESP), .RLAST(RLAST),
    .RUSER(RUSER), .RVALID(RVALID), .RREADY(RREADY), .AWADDR(AWADDR),
    .AWVALID(AWVALID), .AWREADY(AWREADY), .AWID(AWID), .AWLEN(AWLEN),
    .AWSIZE(AWSIZE), .AWBURST(AWBURST), .AWLOCK(AWLOCK), .AWCACHE(AWCACHE),
    .AWPROT(AWPROT), .AWQOS(AWQOS), .AWUSER(AWUSER), .WID(WID), .WDATA(WDATA),
    .WSTRB(WSTRB), .WLAST(WLAST), .WVALID(WVALID), .WREADY(WREADY),
    .WUSER(WUSER), .BID(BID), .BRESP(BRESP), .BVALID(BVALID), .BREADY(BREADY),
    .BUSER(BUSER), .REP_RDY(rep_rdy), .REP_VLD(rep_vld),
    .REP_ADDRESS(rep_address), .REP_COUNT(rep_count),
    .REP_MODE_WR(rep_mode_wr), .REP_ERROR(rep_error), .ENT_RRDY(ent_rrdy),
    .ENT_RVLD(ent_rvld), .ENT_RDT(ent_rdt), .ENT_WRDY(ent_wrdy),
    .ENT_WVLD(ent_wvld), .ENT_WDT(ent_wdt));

endmodule

