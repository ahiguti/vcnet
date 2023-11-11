`timescale 1ns / 1ps

// SPI peripheralとして動作する。配列に保持した値をSPIでcontrollerに渡す。
// CS信号は使わず、一定時間経過でトランザクション終了と見做す。
// そのかわり最初の1bitをcontrollerに渡すことができず、つねに0が渡るので注意。
// WR_*で配列に値を書き込む。

module vcnet_spi_peripheral(
CLK,
RESETN,
SCLK,
DOUT,
VAL_ARRAY,
STAT_INTERVAL
);
parameter ARR_BYTES = 32;
    // 配列の大きさ
parameter REWIND_THRESHOLD = 30000;
    // このクロックサイクル経過すると読み出し位置を巻き戻す
parameter MCU_ID = 0;
    // このインスタンスが操作対象とするマイコンの番号
//parameter DOUT_DELAY = 2;
//    // sclkのrising edigeからdoutを書き換えるまでの遅延。これに加えて同期化のためのサイクルぶんも遅延することに注意。
input CLK;
input RESETN;
input SCLK;
output DOUT;
input [ARR_BYTES*8-1:0] VAL_ARRAY;
output [31:0] STAT_INTERVAL;

wire [7:0] arr[0:ARR_BYTES-1];
genvar k;
generate
    for (k = 0; k < ARR_BYTES; k = k + 1) begin
        assign arr[k] = VAL_ARRAY[k*8+7:k*8];
    end
endgenerate

localparam MAGIC_NUMBER = 32'hDEADBEEF;
localparam MAGIC_NUMBER_SIZE = 4;

(* ASYNC_REG = "TRUE" *) reg [2:0] sync_sclk;

reg [7:0] arr_copy[0:ARR_BYTES-1]; // トランザクション開始時にarrからコピー
reg [7:0] cur_byte;
reg [15:0] cur_byteidx;
reg [2:0] cur_bitidx;
reg dout;
reg [15:0] sclk_low_count; // 連続してsclkがlowだったサイクル数
reg [7:0] sclk_high_count; // 連続してsclkがhighだったサイクル数
reg [7:0] estimated_tsclkhigh; // 前回sclkがhighだったサイクル数。doutをセットするタイミングを決めるのに使う。
reg [31:0] stat_count; // トランザクション間インターバルを数えるカウンタ
reg [31:0] stat_interval; // 直前回のトランザクション間インターバル

assign DOUT = dout;
assign STAT_INTERVAL = stat_interval;

wire sclk = sync_sclk[2];
wire lsb_first = 0; // 既定ではmsb first。

integer i;
always @(posedge CLK) begin
    if (!RESETN) begin
        for (i = 0; i < ARR_BYTES; i = i + 1) begin
            arr_copy[i] <= 0;
        end
        cur_byte <= 0;
        cur_byteidx <= 0;
        cur_bitidx <= 0;
        dout <= 0;
        sclk_low_count <= 0;
        sclk_high_count <= 0;
        estimated_tsclkhigh <= 1;
        stat_count <= 0;
        stat_interval <= 0;
    end else begin
        sync_sclk <= { sync_sclk, SCLK };
        stat_count <= stat_count + 1;
        if (sclk == 0) begin
            sclk_low_count <= (sclk_low_count < 65535) ? (sclk_low_count + 1) : 65535;
            sclk_high_count <= 0;
            if (sclk_high_count > 0 && sclk_high_count < 255) begin
                // sclkがhighだった時間を記録しておく。doutをセットするタイミングを決めるのに使う。
                // sclkは同期化のために3サイクル遅れているのでsclkのfalling edgeの3サイクル前あたり
                // でdoutをセットする。estimated_tsclkhighの既定値は1なので初回のdoutセットは早すぎるが、
                // 初回はdoutをholdしておく必要はない(最初の1bitは無効)ので問題無い。
                estimated_tsclkhigh <= (sclk_high_count > 3) ? (sclk_high_count - 3) : 1;
            end
        end else begin
            sclk_low_count <= 0;
            sclk_high_count <= (sclk_high_count < 255) ? (sclk_high_count + 1) : 255;
        end
        if (sclk_low_count >= REWIND_THRESHOLD) begin
            cur_byte <= 0;
            cur_byteidx <= 0;
            cur_bitidx <= 0;
            dout <= 0;
        end
        if (sclk_high_count == 0) begin
            if (sclk && cur_bitidx == 0 && cur_byteidx == 0) begin
                // トランザクションの最初のsclkのposedge。最初の1bitを渡すタイミングは過ぎているので
                // 次のbitの用意をする。
                // オフセット0の最上位bit(lsb_firstのときは最下位bit)は値を渡すことはできないので注意。
                if (lsb_first) begin
                    cur_byte <= arr[0] >> 1;
                end else begin
                    cur_byte <= arr[0] << 1;
                end
                cur_bitidx <= 1;
                for (i = 0; i < ARR_BYTES; i = i + 1) begin
                    arr_copy[i] <= arr[i];
                end
                stat_interval <= stat_count;
                stat_count <= 0;
            end
        end else if (sclk_high_count == estimated_tsclkhigh) begin
            // sclkのnegedge付近。doutの値を変更し、次の値の準備をする。
            if (lsb_first) begin
                dout <= cur_byte[0];
                cur_byte <= cur_byte >> 1;
            end else begin
                dout <= cur_byte[7];
                cur_byte <= cur_byte << 1;
            end
            if (cur_bitidx != 7) begin
                cur_bitidx <= cur_bitidx + 1;
            end else begin
                cur_bitidx <= 0;
                cur_byteidx <= (cur_byteidx < 65535) ? (cur_byteidx + 1) : (cur_byteidx - 3);
                cur_byte <= get_reg_value(cur_byteidx + 1);
//                cur_byteidx <= (cur_byteidx + 1 < ARR_BYTES) ? (cur_byteidx + 1) : ARR_BYTES;
//                cur_byte <= (cur_byteidx + 1 < ARR_BYTES) ? arr_copy[cur_byteidx + 1] : 0;
//                if (cur_byteidx + 1 < ARR_BYTES) begin
//                    cur_byteidx <= cur_byteidx + 1;
//                    cur_byte <= arr_copy[cur_byteidx + 1];
//                end else begin
//                    cur_byteidx <= ARR_BYTES;
//                    cur_byte <= 0;
//                end
            end
        end
    end
end

function [7:0] get_reg_value(input [15:0] idx);
begin
    get_reg_value = 0;
    if (idx >= ARR_BYTES) begin
        case (idx[1:0])
        0: get_reg_value = MAGIC_NUMBER[7:0];
        1: get_reg_value = MAGIC_NUMBER[15:8];
        2: get_reg_value = MAGIC_NUMBER[23:16];
        3: get_reg_value = MAGIC_NUMBER[31:24];
        endcase
    end else if (MCU_ID == (arr_copy[0] & 8'H7f)) begin
        get_reg_value = arr_copy[idx];
    end
end
endfunction

endmodule
