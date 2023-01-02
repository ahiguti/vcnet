`timescale 1ns / 1ps


// UDPで受け取ったコマンドを読み取って分配する。
// EXT_TDATAは外部マイコンへのi2cコマンド
// ADV7611_TDATAはADV7611へのi2cコマンド
// IR_TDATAはIRコマンド

module vcnet_parse_command(
input CLK,
input RESETN,
input I_TVALID,
input [63:0] I_TDATA,
input [3:0] I_TUSER,
input I_TLAST,
output ADV7611_TVALID,
output [23:0] ADV7611_TDATA,
output EXT_TVALID,
output [31:0] EXT_TDATA,
output EXT2_TVALID,
output [47:0] EXT2_TDATA,
output IR_TVALID,
output [63:0] IR_TDATA,
output [7:0] IR_TSTRB,
output IR_TLAST,
output [7:0] DBG_PACKET
);

reg [7:0] state;
reg i_tvalid;
reg i_tlast;
reg adv7611_tvalid;
reg [23:0] adv7611_tdata;
reg ext_tvalid;
reg [31:0] ext_tdata;
reg ext2_tvalid;
reg [47:0] ext2_tdata;
reg ir_tvalid;
reg [63:0] ir_tdata;
reg [7:0] ir_tstrb;
reg ir_tlast;
reg [7:0] dbg_packet;

assign ADV7611_TVALID = adv7611_tvalid;
assign ADV7611_TDATA = adv7611_tdata;
assign EXT_TVALID = ext_tvalid;
assign EXT_TDATA = ext_tdata;
assign EXT2_TVALID = ext2_tvalid;
assign EXT2_TDATA = ext2_tdata;
assign IR_TVALID = ir_tvalid;
assign IR_TDATA = ir_tdata;
assign IR_TSTRB = ir_tstrb;
assign IR_TLAST = ir_tlast;
assign DBG_PACKET = dbg_packet;

always @(posedge CLK) begin
    i_tvalid <= I_TVALID;
    i_tlast <= I_TLAST;
    adv7611_tvalid <= 0;
    adv7611_tdata <= 0;
    ext_tvalid <= 0;
    ext_tdata <= 0;
    ext2_tvalid <= 0;
    ext2_tdata <= 0;
    ir_tvalid <= 0;
    ir_tdata <= 0;
    ir_tstrb <= 0;
    ir_tlast <= 0;
    dbg_packet <= 0;
    if (!RESETN) begin
        state <= 0;
    end else begin
        if (state == 0) begin
            if (I_TVALID && (!i_tvalid || i_tlast)) begin
                // パケットのお最初のワード
                dbg_packet <= I_TDATA[15:8];
                if (I_TDATA[7:0] == 3 && I_TDATA[15:8] == 5) begin
                    // adv7611へのi2cコマンド
                    adv7611_tvalid <= 1;
                    adv7611_tdata <= I_TDATA[39:16];
                end
                if (I_TDATA[7:0] == 4 && I_TDATA[15:8] == 1) begin
                    // 外部マイコンへのi2cコマンド
                    ext_tvalid <= 1;
                    ext_tdata <= I_TDATA[47:16];
                end
                if (I_TDATA[7:0] == 6 && I_TDATA[15:8] == 6) begin
                    // 外部マイコンへのspiコマンド。8byteに一つのspiコマンドを詰める。
                    // udpの1パケットに複数コマンドを含める。
                    ext2_tvalid <= 1;
                    ext2_tdata <= I_TDATA[63:16];
                    state <= 2;
                end
                if (I_TDATA[7:0] > 8 && I_TDATA[15:8] == 3 && !I_TLAST) begin
                    // IRコマンド。長さ6byte以下のときは無視する。
                   ir_tvalid <= 1;
                   ir_tdata <= { I_TDATA[63:16], 16'h0 };
                   ir_tstrb <= 8'b1111100;
                   state <= 1;
                end
            end
        end else if (state == 1) begin
            // IRコマンド
            if (I_TVALID) begin
                ir_tvalid <= 1;
                ir_tdata <= I_TDATA[63:0];
                ir_tstrb <= (I_TUSER == 8) ? 8'b11111111
                    : (I_TUSER >= 6) ? 8'b00111111
                    : (I_TUSER >= 4) ? 8'b00001111
                    : (I_TUSER >= 2) ? 8'b00000011
                    : 8'b00000000;
                ir_tlast <= I_TLAST;
                if (I_TLAST) begin
                    state <= 0;
                end
            end else begin
                // 連続的でなかった。
                ir_tvalid <= 1;
                ir_tstrb <= 0;
                ir_tdata <= 0;
                ir_tlast <= 1;
                state <= 0;
            end
        end else if (state == 2) begin
            // 外部マイコンへのspiコマンド。
            if (I_TVALID && I_TDATA[7:0] == 6 && I_TDATA[15:8] == 6) begin
                ext2_tdata <= I_TDATA[63:16];
                ext2_tvalid <= 1;
                if (I_TLAST) begin
                    state <= 0;
                end
            end else begin
                ext2_tvalid <= 0;
                ext2_tdata <= 0;
                state <= 0;
            end
        end
    end
end

endmodule
