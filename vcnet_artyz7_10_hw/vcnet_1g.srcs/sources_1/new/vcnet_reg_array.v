`timescale 1ns / 1ps

module vcnet_reg_array(
CLK,
RESETN,
WR_ENABLE,
WR_IDX,
WR_VALUE,
VAL_ARRAY
);
parameter NUM_WORDS = 8;
input CLK;
input RESETN;
input WR_ENABLE;
input [7:0] WR_IDX;
input [31:0] WR_VALUE;
output [NUM_WORDS*32-1:0] VAL_ARRAY;

reg [31:0] arr[0:NUM_WORDS-1];

genvar i;
generate
for (i = 0; i < NUM_WORDS; i = i + 1) begin
    assign VAL_ARRAY[i*32+31:i*32] = arr[i];
end
endgenerate

integer j;
always @(posedge CLK) begin
    if (!RESETN) begin
        for (j = 0; j < NUM_WORDS; j = j + 1) begin
            arr[j] <= 0;
        end
    end else begin
        if (WR_ENABLE) begin
            arr[WR_IDX] = WR_VALUE;
        end
    end
end

endmodule
