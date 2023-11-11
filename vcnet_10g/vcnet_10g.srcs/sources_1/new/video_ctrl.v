
// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module video_ctrl(
input wire CLK,
input wire RESET_N,
output wire READER_AP_START,
input wire READER_AP_DONE,
input wire READER_AP_IDLE,
input wire READER_AP_READY,
output wire [11:0] XSIZE,
output wire [11:0] YSIZE,
output wire [31:0] RD_DRAM_ADDR,
output wire [31:0] WR_DRAM_ADDR,
input wire [31:0] VIN_TDATA,
input wire VIN_TVALID,
output wire VIN_TREADY,
output wire [23:0] VOUT_TDATA,
output wire VOUT_TLAST,
output wire VOUT_TUSER,
output wire VOUT_TVALID,
input wire VOUT_TREADY,
output wire [7:0] DEBUG_OUT,
output wire [31:0] CLOCK_CNT_SAVED
);

reg [2:0] state;
reg reading_idx;
reg [7:0] cnt;
reg [31:0] clock_cnt;
reg [31:0] clock_cnt_saved;

assign READER_AP_START = (state == 2);
assign XSIZE = 1280;
assign YSIZE = 720;
assign RD_DRAM_ADDR = (reading_idx == 0) ? 32'h18000000 : 32'h18400000;
assign WR_DRAM_ADDR = (reading_idx != 0) ? 32'h18000000 : 32'h18400000;
assign VIN_TREADY = VOUT_TREADY;
assign VOUT_TDATA = VIN_TDATA[23:0];
assign VOUT_TVALID = VIN_TVALID;
assign VOUT_TLAST = VIN_TDATA[24];
assign VOUT_TUSER = VIN_TDATA[25];
// assign DEBUG_OUT = { CLK, RESET_N, READER_AP_READY, READER_AP_IDLE, READER_AP_DONE, state };
assign DEBUG_OUT = { cnt, CLK, reading_idx };
assign CLOCK_CNT_SAVED = clock_cnt_saved;

always @(posedge CLK) begin
    if (!RESET_N) begin
        state <= 0;
        reading_idx <= 0;
        cnt <= 0;
        clock_cnt <= 0;
        clock_cnt_saved <= 0;
    end else begin
        clock_cnt <= clock_cnt + 1;
        if (state == 0 && READER_AP_IDLE) begin
            state <= 1;
        end else if (state == 1) begin
            state <= 2;
        end else if (state == 2 && READER_AP_DONE) begin
            state <= 3;
        end else if (state == 3) begin
            reading_idx <= !reading_idx;
            cnt <= cnt + 1;
            clock_cnt_saved <= clock_cnt;
            clock_cnt <= 0;
            state <= 2;
        end
    end
end

endmodule
