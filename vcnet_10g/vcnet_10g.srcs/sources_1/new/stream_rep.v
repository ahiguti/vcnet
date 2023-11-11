
// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module stream_rep(
CLK, RESET_N, TOAXI_VLD, TOAXI_RDY, TOAXI_DATA, READTR_READY,
FROMAXI_VLD, FROMAXI_RDY, FROMAXI_DATA, REP_RDY, REP_VLD, REP_ADDRESS,
REP_COUNT, REP_MODE_WR, REP_ERROR, ENT_RRDY, ENT_RVLD, ENT_RDT, ENT_WRDY,
ENT_WVLD, ENT_WDT
);
parameter DATA_BITS = 32;
input wire CLK;
input wire RESET_N;
input wire TOAXI_VLD;
output wire TOAXI_RDY;
input wire [DATA_BITS-1:0] TOAXI_DATA;
input wire READTR_READY; // read transaction is started only if READTR_READY
output wire FROMAXI_VLD;
input wire FROMAXI_RDY;
output wire [DATA_BITS-1:0] FROMAXI_DATA;
input wire REP_RDY;
output wire REP_VLD;
output wire [31:0] REP_ADDRESS;
output wire REP_MODE_WR;
output wire [31:0] REP_COUNT;
input wire REP_ERROR;
input wire ENT_RRDY;
output wire ENT_RVLD;
output wire [DATA_BITS-1:0] ENT_RDT;
output wire ENT_WRDY;
input wire ENT_WVLD;
input wire [DATA_BITS-1:0] ENT_WDT;

reg state_ini;
reg state_sec;
reg state_wait_readreq;
reg state_starttr;
reg state_readtr;
reg state_writetr;
reg state_resp;
reg [31:0] address;
reg [31:0] len;
reg mode_write;
reg need_resp;
reg [31:0] resp;

assign TOAXI_RDY = state_ini || state_sec || (state_writetr && ENT_RRDY);
assign FROMAXI_VLD = state_resp || (state_readtr && ENT_WVLD);
assign FROMAXI_DATA = state_resp ? resp : ENT_WDT;
assign REP_VLD = state_starttr;

assign REP_ADDRESS = address;
assign REP_COUNT = len;
assign REP_MODE_WR = mode_write;
assign ENT_RVLD = state_writetr && TOAXI_VLD;
assign ENT_RDT = TOAXI_DATA;
assign ENT_WRDY = state_readtr && FROMAXI_RDY;

always @(posedge CLK) begin
    if (!RESET_N) begin
        state_ini <= 1;
        state_sec <= 0;
	state_wait_readreq <= 0;
        state_starttr <= 0;
        state_readtr <= 0;
        state_writetr <= 0;
        state_resp <= 0;
        address <= 0;
        len <= 0;
        mode_write <= 0;
        need_resp <= 0;
        resp <= 0;
    end else begin
        if (state_ini && TOAXI_VLD) begin
            address <= { TOAXI_DATA[29:0], 2'b0 };
            mode_write <= TOAXI_DATA[31];
            need_resp <= TOAXI_DATA[30];
            state_ini <= 0;
            state_sec <= 1;
        end
        if (state_sec && TOAXI_VLD) begin
            len <= TOAXI_DATA[29:0];
            state_sec <= 0;
            if (mode_write) begin
                state_starttr <= 1;
	       end else begin
                state_wait_readreq <= 1;
	       end
        end
        if (state_wait_readreq && READTR_READY) begin
            state_wait_readreq <= 0;
            state_starttr <= 1;
        end
        if (state_starttr && REP_RDY) begin
            state_writetr <= mode_write;
            state_readtr <= !mode_write;
            state_starttr <= 0;
        end
        if ((state_writetr || state_readtr) && REP_RDY) begin
            resp <= REP_ERROR;
            state_writetr <= 0;
            state_readtr <= 0;
            if (need_resp) begin
                state_resp <= 1;
            end else begin
                state_ini <= 1;
            end
        end
        if (state_resp && FROMAXI_RDY) begin
            state_resp <= 0;
            state_ini <= 1;
        end
    end
end

endmodule

