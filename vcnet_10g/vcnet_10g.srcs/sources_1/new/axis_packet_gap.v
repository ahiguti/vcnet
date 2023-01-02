`timescale 1ns / 1ps

module axis_packet_gap(
input CLK,
input RESETN,
input I_TVALID,
output I_TREADY,
input [63:0] I_TDATA,
input [3:0] I_TUSER,
input I_TLAST,
output O_TVALID,
input O_TREADY,
output [63:0] O_TDATA,
output [3:0] O_TUSER,
output O_TLAST
);

reg [1:0] gap;

assign I_TREADY = O_TREADY && (gap == 0);
assign O_TVALID = I_TVALID && (gap == 0);
assign O_TDATA = I_TDATA;
assign O_TUSER = (I_TVALID != 0) ? I_TUSER : 0;
assign O_TLAST = I_TLAST;

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
