
// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module hdmi_init(
input wire CLK,
input wire RESET_N,
input wire REINIT,
input wire I2CM_READY,
output wire I2CM_VALID,
output wire [6:0] I2CM_DEVADDR,
output wire [7:0] I2CM_REGADDR,
output wire [31:0] I2CM_WDATA,
output wire [2:0] I2CM_WBYTES,
input wire [31:0] I2CM_RDATA,
output wire [2:0] I2CM_RBYTES,
output wire I2CM_SEND_NACK,
input wire [2:0] I2CM_ERR,
output wire [3:0] DEBUG_STATE,
output wire [4:0] DEBUG_IDX
);

localparam devaddr = 6'h39;

reg [4:0] idx;
reg [3:0] state;
//reg [15:0] init_pairs[0:29];
reg i2cm_valid;
reg [7:0] i2cm_regaddr;
reg [7:0] i2cm_wdata;
reg [2:0] i2cm_wbytes;
reg [2:0] i2cm_rbytes;
reg [31:0] delay_count;
reg hpd;

assign I2CM_VALID = i2cm_valid;
assign I2CM_DEVADDR = devaddr;
assign I2CM_REGADDR = i2cm_regaddr;
assign I2CM_WDATA = i2cm_wdata;
assign I2CM_WBYTES = i2cm_wbytes;
assign I2CM_RBYTES = i2cm_rbytes;
assign I2CM_SEND_NACK = 1;
assign DEBUG_STATE = state;
assign DEBUG_IDX = idx;

/*
initial begin
    init_pairs[00] = 16'h1501;
    init_pairs[01] = 16'h1638;
    init_pairs[02] = 16'h18e7;
    init_pairs[03] = 16'h1934;
    init_pairs[04] = 16'h1bad;
    init_pairs[05] = 16'h1f1b;
    init_pairs[06] = 16'h201d;
    init_pairs[07] = 16'h21dc;
    init_pairs[08] = 16'h23ad;
    init_pairs[09] = 16'h241f;
    init_pairs[10] = 16'h2524;
    init_pairs[11] = 16'h2601;
    init_pairs[12] = 16'h2735;
    init_pairs[13] = 16'h2bad;
    init_pairs[14] = 16'h2d7c;
    init_pairs[15] = 16'h2f77;
    init_pairs[16] = 16'h4110;
    init_pairs[17] = 16'h4808;
    init_pairs[18] = 16'h4c04;
    init_pairs[19] = 16'h5512;
    init_pairs[20] = 16'h9803;
    init_pairs[21] = 16'h9ae0;
    init_pairs[22] = 16'h9c30;
    init_pairs[23] = 16'h9d61;
    init_pairs[24] = 16'ha2a4;
    init_pairs[25] = 16'ha3a4;
    init_pairs[26] = 16'haf04;
    init_pairs[27] = 16'hd03c;
    init_pairs[28] = 16'he0d0;
    init_pairs[29] = 16'hf900;
end
*/

function [15:0] init_reg(input [4:0] idx);
begin
    case (idx)
//    00: init_reg = 16'h4150;
    00: init_reg = 16'h1501;
    01: init_reg = 16'h1501;
    02: init_reg = 16'h16bd;
    03: init_reg = 16'h1702;
    04: init_reg = 16'h1846;
    05: init_reg = 16'h4080;
    06: init_reg = 16'h4110;
    07: init_reg = 16'h4808;
    08: init_reg = 16'h49a8;
    09: init_reg = 16'h4c00;
    10: init_reg = 16'h5520;
    11: init_reg = 16'h5608;
    12: init_reg = 16'h9620;
    13: init_reg = 16'h9803;
    14: init_reg = 16'h9902;
    15: init_reg = 16'h9ae0;
    16: init_reg = 16'h9c30;
    17: init_reg = 16'h9d61;
    18: init_reg = 16'ha2a4;
    19: init_reg = 16'ha3a4;
    20: init_reg = 16'ha544;
    21: init_reg = 16'hab40;
    22: init_reg = 16'haf06;
    23: init_reg = 16'hba00;
    24: init_reg = 16'hd03c;
    25: init_reg = 16'hd1ff;
    26: init_reg = 16'hde9c;
    27: init_reg = 16'he0d0;
    28: init_reg = 16'he460;
    29: init_reg = 16'hf900;
    30: init_reg = 16'hfa00;
    31: init_reg = 16'hfa00;
    endcase
end
endfunction

always @(posedge CLK) begin
    if (!RESET_N) begin
        state <= 0;
        idx <= 0;
        i2cm_valid <= 0;
        i2cm_regaddr <= 0;
        i2cm_wdata <= 0;
        i2cm_wbytes <= 0;
        i2cm_rbytes <= 0;
        delay_count <= 0;
        hpd <= 1;
    end
    if (state == 0 && I2CM_READY) begin
        state <= 1;
        i2cm_valid <= 1;
        i2cm_regaddr <= init_reg(idx) >> 8; // init_pairs[idx][15:8];
        i2cm_wdata <= init_reg(idx); // init_pairs[idx][7:0];
        i2cm_wbytes <= 1;
        i2cm_rbytes <= 0;
    end
    if (state == 1 && !I2CM_READY) begin
        i2cm_valid <= 0;
        if (idx != 30) begin
            state <= 0;
            idx <= idx + 1;
        end else begin
            state <= 2;
            idx <= 0;
        end
    end
    if (state == 2 && REINIT) begin
        state <= 0;
        idx <= 0;
    end
    if (state == 2 && I2CM_READY) begin
        state <= 3;
        i2cm_valid <= 1;
        i2cm_regaddr <= 8'h42; // bit6: HPD state
        i2cm_wbytes <= 0;
        i2cm_rbytes <= 1;
        delay_count <= 1024 * 1024;
    end
    if (state == 3 && !I2CM_READY) begin
        i2cm_valid <= 0;
        state <= 4;
    end
    if (state == 4 && I2CM_READY) begin
        delay_count <= delay_count - 1;
        if (delay_count == 0) begin
            state <= 2;
        end else begin
            hpd <= I2CM_RDATA[6];
            if (I2CM_RDATA[6] == 1 && hpd == 0) begin
                state <= 0;
            end
        end
    end
end

endmodule
