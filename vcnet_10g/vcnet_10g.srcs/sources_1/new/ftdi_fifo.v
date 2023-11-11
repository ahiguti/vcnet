// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

// FT245 synchronous mode


module ftdi_fifo(
FTDI_CLK, FTDI_CLK_DO, FTDI_RESET_N, FTDI_DATA, FTDI_BE, FTDI_RXF_N,
FTDI_TXE_N, FTDI_WR_N, FTDI_RD_N, FTDI_OE_N, FTDI_SIWU_N, FF_CMD_PUSH,
FF_CMD_PEEK, FF_CMD_POP, FF_CMD_PRST, FF_WDATA, FF_RDATA, FF_CUR_PUSHLEN,
FF_CUR_PEEKLEN, FF_CUR_POPLEN, FF_CUR_NOPUSH, FF_CUR_NOPEEK, FF_CUR_NOPOP,
FF_READTR_READY, DEBUG_SW, DEBUG_LD);

parameter DATA_BITS = 32;
parameter BE_BITS = 4;
parameter FF_OUT_ADDR_BITS = 4;
parameter FF_IN_ADDR_BITS = 8;
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
output FF_CMD_PUSH;
output FF_CMD_PEEK;
output FF_CMD_POP;
output FF_CMD_PRST;
output [DATA_BITS-1:0] FF_WDATA;
input [DATA_BITS-1:0] FF_RDATA;
input [FF_OUT_ADDR_BITS:0] FF_CUR_PUSHLEN;
input [FF_IN_ADDR_BITS:0] FF_CUR_PEEKLEN;
input [FF_IN_ADDR_BITS:0] FF_CUR_POPLEN;
input FF_CUR_NOPUSH;
input FF_CUR_NOPEEK;
input FF_CUR_NOPOP;
output FF_READTR_READY;
input [7:0] DEBUG_SW;
output [7:0] DEBUG_LD;

(* keep="true", equivalent_register_removal="no" *) reg [DATA_BITS-1:0] data_out;
(* keep="true", equivalent_register_removal="no" *) reg [DATA_BITS-1:0] data_dir;
(* keep="true", equivalent_register_removal="no" *) reg [BE_BITS:0] be_out;
(* keep="true", equivalent_register_removal="no" *) reg [BE_BITS:0] be_dir;
(* keep="true", equivalent_register_removal="no" *) reg [DATA_BITS-1:0] data_in_r;
(* keep="true", equivalent_register_removal="no" *) reg [BE_BITS:0] be_in_r;
(* keep="true", equivalent_register_removal="no" *) reg wr_n;
(* keep="true", equivalent_register_removal="no" *) reg rd_n;
(* keep="true", equivalent_register_removal="no" *) reg oe_n;
reg rxf_n_r;
reg txe_n_r;
(* keep="true", equivalent_register_removal="no" *) reg [DATA_BITS-1:0] data_dir_pre;
(* keep="true", equivalent_register_removal="no" *) reg [DATA_BITS-1:0] be_dir_pre;
reg wr_n_pre;
reg rd_n_pre;
reg oe_n_pre;

reg [3:0] state;
reg [1:0] ff_cmd;
reg [DATA_BITS-1:0] ff_wdata;
reg [DATA_BITS-1:0] ff_rdata;
reg [3:0] cur_nopeek_r;
reg [1:0] rd_n_pre_r;
reg [1:0] wr_n_pre_r;

reg [31:0] read_cnt;
reg [31:0] write_cnt;
reg [7:0] cmd_cnt;
reg [7:0] reset_cnt;
reg [DATA_BITS-1:0] dbg_rdata;
reg [7:0] debug_sw_r;
reg [7:0] debug_ld_r;
reg [7:0] debug_ld_rr;
reg [7:0] debug_out0;
reg [7:0] debug_out1;
reg [7:0] debug_out2;
reg [7:0] debug_out3;
reg [7:0] debug_out4;
reg [7:0] debug_out5;
reg [7:0] debug_out6;
reg [7:0] debug_out7;

assign FF_CMD_PUSH = ff_cmd == 1;
assign FF_CMD_PEEK = ff_cmd == 2;
assign FF_CMD_POP = ff_cmd == 3;
assign FF_CMD_PRST = ff_cmd == 0;
assign FF_WDATA = ff_wdata;
assign FF_READTR_READY = !txe_n_r; // true when usb requests data to read

wire [DATA_BITS-1:0] data_in;
wire [BE_BITS-1:0] be_in;

wire [7:0] DEBUG_OUT0;
wire [7:0] DEBUG_OUT1;
wire [7:0] DEBUG_OUT2;
wire [7:0] DEBUG_OUT3;
wire [7:0] DEBUG_OUT4;
wire [7:0] DEBUG_OUT5;
wire [7:0] DEBUG_OUT6;
wire [7:0] DEBUG_OUT7;

assign DEBUG_LD = ENABLE_DEBUG_OUT ? debug_ld_rr: 0;

genvar i;
generate
for (i = 0; i < DATA_BITS; i = i + 1) begin : loop0
IOBUF iob_data(.IO(FTDI_DATA[i]), .T(data_dir[i]), .I(data_out[i]), .O(data_in[i]));
end
for (i = 0; i < (BE_BITS > 1 ? BE_BITS : 0); i = i + 1) begin : loop1
IOBUF iob_be(.IO(FTDI_BE[i]), .T(be_dir[i]), .I(be_out[i]), .O(be_in[i]));
end
endgenerate

assign FTDI_WR_N = wr_n;
assign FTDI_RD_N = rd_n;
assign FTDI_OE_N = oe_n;
assign FTDI_SIWU_N = 1'b1;

wire debug_nolimit_mode = debug_sw_r[7];
wire next_wr_n_pre = FF_CUR_NOPEEK && !debug_nolimit_mode;

always @(posedge FTDI_CLK_DO) begin
    data_dir <= data_dir_pre;
    data_out <= ff_rdata;
    be_dir <= data_dir_pre;
    be_out <= 4'b1111;
    wr_n <= wr_n_pre;
    oe_n <= oe_n_pre;
    rd_n <= rd_n_pre;
end

always @(posedge FTDI_CLK) begin
    debug_sw_r <= DEBUG_SW;
    ff_rdata <= FF_RDATA;
    debug_out0 <= { be_in_r, state[3:0] };
    debug_out1 <= ff_wdata;
    debug_out2 <= read_cnt;
    debug_out3 <= write_cnt;
    debug_out4 <= read_cnt[15:8];
    debug_out5 <= write_cnt[15:8];
    debug_out6 <= FF_CUR_PUSHLEN > 255 ? 255 : FF_CUR_PUSHLEN; // ff_wdata;
    debug_out7 <= FF_CUR_POPLEN > 255 ? 255 : FF_CUR_POPLEN; // ff_wdata[15:8];
    debug_ld_r <= debug_sw_r[2]
    ? (debug_sw_r[1] ? (debug_sw_r[0] ? debug_out7 : debug_out6) : (debug_sw_r[0] ? debug_out5 : debug_out4))
    : (debug_sw_r[1] ? (debug_sw_r[0] ? debug_out3 : debug_out2) : (debug_sw_r[0] ? debug_out1 : debug_out0));
    debug_ld_rr <= debug_ld_r;
    rxf_n_r <= FTDI_RXF_N;
    txe_n_r <= FTDI_TXE_N;
    data_in_r <= data_in;
    be_in_r <= BE_BITS > 1 ? be_in : 1;
    cur_nopeek_r <= { cur_nopeek_r, FF_CUR_NOPEEK };
    rd_n_pre_r <= { rd_n_pre_r, rd_n_pre };
    wr_n_pre_r <= { wr_n_pre_r, wr_n_pre };
    ff_cmd <= 0; // PRSET
    if (FTDI_RESET_N == 0) begin
        state <= 0;
        wr_n_pre <= 1;
        rd_n_pre <= 1;
        oe_n_pre <= 1;
        data_dir_pre <= 32'hffffffff;
        be_dir_pre <= 4'b1111;
        read_cnt <= 0;
        write_cnt <= 0;
        ff_cmd <= 0;
        ff_wdata <= 0;
        cmd_cnt <= 0;
        dbg_rdata <= 0;
        reset_cnt <= reset_cnt + 1;
    end else if (state == 0) begin
        if (rxf_n_r == 0 && (FF_CUR_NOPUSH == 0 || debug_nolimit_mode)) begin
            // master read, usb write
            oe_n_pre <= 0;
//            cmd_cnt <= cmd_cnt + 1;
            state <= 1;
        end else if (txe_n_r == 0 &&
            (FF_CUR_PEEKLEN != 0 || debug_nolimit_mode)) begin
            // master write, usb read
            ff_cmd <= 2; // PEEK
                // at least one data is available in fifo_reg
            state <= 9;
        end
    end else if (state == 1) begin
        rd_n_pre <= 0;
        state <= state + 1;
    end else if (state == 2) begin
        oe_n_pre <= rd_n_pre || (FF_CUR_PUSHLEN <= 3 && !debug_nolimit_mode);
        rd_n_pre <= rd_n_pre || (FF_CUR_PUSHLEN <= 3 && !debug_nolimit_mode);
        state <= state + 1;
    end else if (state == 3) begin
        oe_n_pre <= rd_n_pre || (FF_CUR_PUSHLEN <= 3 && !debug_nolimit_mode);
        rd_n_pre <= rd_n_pre || (FF_CUR_PUSHLEN <= 3 && !debug_nolimit_mode);
        if (rxf_n_r == 0 && rd_n_pre_r[0] == 0) begin
            ff_wdata <= data_in_r;
            ff_cmd <= 1; // PUSH
            read_cnt <= read_cnt + 1;
        end else begin
            oe_n_pre <= 1;
            rd_n_pre <= 1;
            ff_cmd <= 0; // PRSET
            state <= 0;
        end
    end else if (state == 9) begin
        ff_cmd <= 2; // PEEK
        wr_n_pre <= next_wr_n_pre;
        data_dir_pre <= 0; // master write
        be_dir_pre <= 0;
        state <= state + 1;
    end else if (state == 10) begin
        ff_cmd <= 2; // PEEK
        wr_n_pre <= next_wr_n_pre;
        data_dir_pre <= next_wr_n_pre ? 32'hffffffff : 0;
        be_dir_pre <= next_wr_n_pre ? 4'b1111 : 0;
        state <= state + 1;
    end else if (state == 11) begin
        if (txe_n_r == 0 && wr_n_pre_r[0] == 0) begin
            ff_cmd <= 3; // POP
            wr_n_pre <= next_wr_n_pre;
            data_dir_pre <= next_wr_n_pre ? 32'hffffffff : 0;
            be_dir_pre <= next_wr_n_pre ? 4'b1111 : 0;
            write_cnt <= write_cnt + 1;
            dbg_rdata <= ff_rdata;
        end else begin
            ff_cmd <= 0; // PRST
            cmd_cnt <= cmd_cnt == 254 ? 254 : cmd_cnt + 1;
            wr_n_pre <= 1;
            data_dir_pre <= 32'hffffffff;
            be_dir_pre <= 4'b1111;
            state <= state + 1;
        end
    end else if (state == 12) begin
        state <= state + 1;
    end else if (state == 13) begin
        state <= 0;
    end
end

endmodule

