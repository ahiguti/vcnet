`timescale 1ns / 1ps

module vcnet_spi_peripheral_multi(
CLK,
RESETN,
SCLK,
DOUT,
VAL_ARRAY,
STAT_INTERVAL
);
parameter NUM = 5;
    // SPI peripheralのインスタンス数
parameter ARR_BYTES = 32;
    // 配列の大きさ
parameter REWIND_THRESHOLD = 30000;
    // このクロックサイクル経過すると読み出し位置を巻き戻す
input CLK;
input RESETN;
input [NUM-1:0] SCLK;
output [NUM-1:0] DOUT;
input [ARR_BYTES*8-1:0] VAL_ARRAY;
output [31:0] STAT_INTERVAL;

wire [31:0] stat_interval_inst[0:NUM-1];
assign STAT_INTERVAL = stat_interval_inst[0];

genvar i;
generate
for (i = 0; i < NUM; i = i + 1) begin
    vcnet_spi_peripheral #(.ARR_BYTES(ARR_BYTES), .REWIND_THRESHOLD(REWIND_THRESHOLD), .MCU_ID(i))
      inst(.CLK(CLK), .RESETN(RESETN), .SCLK(SCLK[i]), .DOUT(DOUT[i]), .VAL_ARRAY(VAL_ARRAY), .STAT_INTERVAL(stat_interval_inst[i])); 
end
endgenerate

endmodule
