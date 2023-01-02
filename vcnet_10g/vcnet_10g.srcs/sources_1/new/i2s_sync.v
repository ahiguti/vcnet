`timescale 1ns / 1ps

module i2s_sync(
input CLK,
input I_SCLK,
input I_LRCLK,
input I_AP,
output O_SCLK,
output O_LRCLK,
output O_AP
);

reg [2:0] sclk_sync;
reg [2:0] lrclk_sync;
reg [2:0] ap_sync;

assign O_SCLK = sclk_sync[2];
assign O_LRCLK = lrclk_sync[2];
assign O_AP = ap_sync[2];

always @(posedge CLK) begin
    sclk_sync <= { sclk_sync, I_SCLK };
    lrclk_sync <= { lrclk_sync, I_LRCLK };
    ap_sync <= {ap_sync, I_AP };
end

endmodule
