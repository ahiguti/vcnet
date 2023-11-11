
// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module rep_aximaster(
CLK, RESET_N, ARADDR, ARVALID, ARREADY, ARID, ARLEN, ARSIZE, ARBURST, ARLOCK,
ARCACHE, ARPROT, ARQOS, RID, RDATA, RRESP, RLAST, RUSER, RVALID, RREADY,
AWADDR, AWVALID, AWREADY, AWID, AWLEN, AWSIZE, AWBURST, AWLOCK, AWCACHE,
AWPROT, AWQOS, AWUSER, WID, WDATA, WSTRB, WLAST, WVALID, WREADY, WUSER, BID,
BRESP, BVALID, BREADY, BUSER, REP_RDY, REP_VLD, REP_ADDRESS, REP_COUNT,
REP_MODE_WR, REP_ERROR, ENT_RRDY, ENT_RVLD, ENT_RDT, ENT_WRDY, ENT_WVLD,
ENT_WDT, DEBUG_OUT
);
parameter DATA_BITS = 32;
input wire CLK;
input wire RESET_N;
output wire REP_RDY;
input wire REP_VLD;
input wire [31:0] REP_ADDRESS;
input wire [31:0] REP_COUNT;
input wire REP_MODE_WR;
output wire [1:0] REP_ERROR;
output wire ENT_RRDY;
input wire ENT_RVLD;
input wire [DATA_BITS-1:0] ENT_RDT;
input wire ENT_WRDY;
output wire ENT_WVLD;
output wire [DATA_BITS-1:0] ENT_WDT;
output wire [31:0] ARADDR;
output wire ARVALID;
input wire ARREADY;
output wire [0:0] ARID;
output wire [7:0] ARLEN;
output wire [2:0] ARSIZE;
output wire [1:0] ARBURST;
output wire [1:0] ARLOCK;
output wire [3:0] ARCACHE;
output wire [2:0] ARPROT;
output wire [3:0] ARQOS;
input wire [0:0] RID;
input wire [DATA_BITS-1:0] RDATA;
input wire [1:0] RRESP;
input wire RLAST;
input wire [0:0] RUSER;
input wire RVALID;
output wire RREADY;
output wire [31:0] AWADDR;
output wire AWVALID;
input wire AWREADY;
output wire [0:0] AWID;
output wire [7:0] AWLEN;
output wire [2:0] AWSIZE;
output wire [1:0] AWBURST;
output wire [1:0] AWLOCK;
output wire [3:0] AWCACHE;
output wire [2:0] AWPROT;
output wire [3:0] AWQOS;
output wire [0:0] AWUSER;
output wire [0:0] WID;
output wire [DATA_BITS-1:0] WDATA;
output wire [7:0] WSTRB;
output wire WLAST;
output wire WVALID;
input wire WREADY;
output wire [0:0] WUSER;
input wire [0:0] BID;
input wire [1:0] BRESP;
input wire BVALID;
output wire BREADY;
input wire [0:0] BUSER;
output wire [7:0] DEBUG_OUT;

reg state_fe;
reg state_e;
reg state_ar;
reg state_r;
reg state_aw;
reg state_w;
reg state_b0;
reg state_b1;

reg [31:0] address;
reg [23:0] rep_count;
reg [7:0] burst_len;
reg [7:0] last_burst;
reg [1:0] error;

assign REP_RDY = state_fe;
assign REP_ERROR = error;
assign ENT_RRDY = state_w && WREADY;
assign ENT_WVLD = state_r && RVALID;
assign ENT_WDT = RDATA;

assign ARADDR = address;
assign ARVALID = state_ar;
assign ARID = 0;
assign ARLEN = burst_len;
assign ARSIZE = DATA_BITS == 64 ? 3 : 2; // 8bytes or 4bytes
assign ARBURST = 2'b01; // 0: fixed address, 1: incr, 2: wrap around
assign ARLOCK = 1'b0;
assign ARCACHE = 4'b0011;
assign ARPROT = 3'b0;
assign ARQOS = 4'b0;
assign RREADY = state_r && ENT_WRDY;
assign AWADDR = address;
assign AWVALID = state_aw;
assign AWID = 6'b0;
assign AWLEN = burst_len;
assign AWSIZE = DATA_BITS == 64 ? 3 : 2; // 8bytes or 4bytes
assign AWBURST = 2'b01; // 0: fixed address, 1: incr, 2: wrap around
assign AWLOCK = 2'b0;
assign AWCACHE = 4'b0011;
assign AWPROT = 3'b0;
assign AWQOS = 4'b0;
assign AWUSER = 0;
assign WID = 6'b0;
assign WDATA = ENT_RDT;
assign WSTRB = 8'hff;
assign WLAST = burst_len == 0;
assign WVALID = state_w && ENT_RVLD;
assign WUSER = 0;
assign BREADY = state_b0 && state_b1;

assign DEBUG_OUT = {
  state_fe, state_e, state_ar, state_r, state_aw, state_w, state_b0, state_b1
};

always @(posedge CLK) begin
    if (!RESET_N) begin
        state_fe <= 1;
        state_e <= 0;
        state_ar <= 0;
        state_r <= 0;
        state_aw <= 0;
        state_w <= 0;
        state_b0 <= 0;
        state_b1 <= 0;
    end
    if (REP_RDY && REP_VLD) begin
        state_fe <= 0;
        error <= 0;
        address <= REP_ADDRESS;
        rep_count <= REP_COUNT[31:8];
        burst_len <= (REP_COUNT[31:8] != 0) ? 8'hff : REP_COUNT[7:0];
        last_burst <= REP_COUNT[7:0];
        if (REP_MODE_WR) begin
            state_aw <= 1;
            state_w <= 1;
        end else begin
            state_ar <= 1;
        end
    end
    if (ARVALID && ARREADY) begin
        state_ar <= 0;
        state_r <= 1;
    end
    if (RVALID && RREADY) begin
        state_r <= 0;
        error <= error != 0 ? error : RRESP;
        address <= address + (DATA_BITS / 8);
        if (burst_len != 0 && !RLAST) begin
            burst_len <= burst_len - 1;
            state_r <= 1;
        end else if (rep_count != 0) begin
            rep_count <= rep_count - 1;
            burst_len <= (rep_count != 1) ? 8'hff : last_burst;
            state_ar <= 1;
        end else begin
            state_fe <= 1;
        end
    end
    if (AWVALID && AWREADY) begin
        state_aw <= 0;
        state_b0 <= 1;
    end
    if (WVALID && WREADY) begin
        state_w <= 0;
        address <= address + (DATA_BITS / 8);
        if (burst_len != 0 && !WLAST) begin
            burst_len <= burst_len - 1;
            state_w <= 1;
        end else begin
            state_b1 <= 1;
        end
    end
    if (BVALID && BREADY) begin
        error <= error != 0 ? error : BRESP;
        state_b0 <= 0;
        state_b1 <= 0;
        if (rep_count != 0) begin
            rep_count <= rep_count - 1;
            burst_len <= (rep_count != 1) ? 8'hff : last_burst;
            state_aw <= 1;
            state_w <= 1;
        end else begin
            state_fe <= 1;
        end
    end
end

endmodule

