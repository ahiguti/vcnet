`timescale 1ns / 1ps

// axi-streamからxgmiiへ変換する。パケット間ギャップを確保するためにTREADYで制御する。

module axis_to_xgmii(
input CLK,
input RESETN,
input I_TVALID,
output I_TREADY,
input [63:0] I_TDATA,
input [3:0] I_TUSER,
input I_TLAST,
output [63:0] O_DATA,
output [3:0] O_LEN
);

reg [1:0] gap;

assign I_TREADY = (gap == 0);
assign O_DATA = I_TDATA;
assign O_LEN = (I_TVALID != 0 && gap == 0) ? I_TUSER : 0;

always @(posedge CLK) begin
    if (!RESETN) begin
        gap <= 0;
    end else begin
        if (gap != 0) begin
            gap = gap - 1;
        end
        if (I_TREADY && I_TVALID && I_TLAST) begin
            gap <= 2;
        end
    end
end

endmodule
