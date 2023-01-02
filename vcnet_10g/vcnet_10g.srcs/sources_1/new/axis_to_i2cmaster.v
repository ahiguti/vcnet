
// Copyright (C) Akira Higuchi  ( https://github.com/ahiguti )
// Copyright (C) DeNA Co.,Ltd. ( https://dena.com )
// All rights reserved.
// See COPYRIGHT.txt for details


// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module axis_to_i2cmaster(
input CLK,
input RESET_N,
input I2CM_READY,
output I2CM_VALID,
output [6:0] I2CM_DEVADDR,
output [7:0] I2CM_REGADDR,
output [31:0] I2CM_WDATA,
output [2:0] I2CM_WBYTES,
input [31:0] I2CM_RDATA,
output [2:0] I2CM_RBYTES,
output I2CM_SEND_NACK,
input [2:0] I2CM_ERR,
input I_TVALID,
output I_TREADY,
input [31:0] I_TDATA,
output [3:0] DEBUG_STATE,
output [8:0] DEBUG_IDX
);

reg [8:0] idx;
reg [3:0] state;
reg [31:0] delay_cnt;
reg i2cm_valid;
reg [7:0] i2cm_devaddr;
reg [7:0] i2cm_regaddr;
reg [31:0] i2cm_wdata;

assign I2CM_VALID = i2cm_valid;
assign I2CM_DEVADDR = i2cm_devaddr;
assign I2CM_REGADDR = i2cm_regaddr;  // i2cへ書き込む最初の1byte
assign I2CM_WDATA = i2cm_wdata;      // regaddrに続いて書き込むデータ
assign I2CM_WBYTES = 2;              // regaddrに続いて書き込むデータの長さ
assign I2CM_RBYTES = 0;
assign I2CM_SEND_NACK = 1;
assign DEBUG_STATE = state;
assign DEBUG_IDX = idx;
assign I_TREADY = ((state == 4) && I2CM_READY);

always @(posedge CLK) begin
    if (!RESET_N) begin
        state <= 3; // 初期化のためのi2cコマンドが無いときは3から開始する。
        delay_cnt <= 0;
        idx <= 0;
        i2cm_valid <= 0;
        i2cm_devaddr <= 0;
        i2cm_regaddr <= 0;
        i2cm_wdata <= 0;
    end else begin
        if (state == 0) begin
            if (delay_cnt < 10) begin
                delay_cnt <= delay_cnt + 1;
            end else begin
                delay_cnt <= 0;
                state <= 1;
            end
        end
        if (state == 1 && I2CM_READY) begin
            i2cm_valid <= 1;
            i2cm_devaddr <= init_reg(idx) >> 17; // 7bit address
            i2cm_regaddr <= init_reg(idx) >> 8;
            i2cm_wdata <= init_reg(idx);
            state <= 2;
        end
        if (state == 2 && !I2CM_READY) begin
            i2cm_valid <= 0;
            if ((init_reg(idx + 1) >> 16) != 0) begin
                delay_cnt <= 0;
                idx <= idx + 1;
                state <= 0;
            end else begin
                state <= 3;
            end
        end
        if (state == 3) begin
            if (delay_cnt <= 10) begin
                delay_cnt <= delay_cnt + 1;
            end else begin
                delay_cnt <= 0;
                state <= 4;
            end
        end
        if (state == 4 && I_TVALID && I_TREADY) begin
            i2cm_valid <= 1;
            i2cm_devaddr <= I_TDATA[6:0]; // 7bit address
            i2cm_regaddr <= I_TDATA[15:8];
            i2cm_wdata <= I_TDATA[31:16];
            state <= 5;
        end
        if (state == 5 && !I2CM_READY) begin
            i2cm_valid <= 0;
            state <= 3;
        end
    end
end

function [23:0] init_reg(input [8:0] idx);
begin
init_reg = 0;
end
endfunction

endmodule
