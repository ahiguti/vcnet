`timescale 1ns / 1ps

// audioフレームにTUSER(長さ)を付ける。常に8byte。

module axis_audio_add_len(
input CLK,
input I_TVALID,
output I_TREADY,
input [63:0] I_TDATA,
input I_TLAST,
output O_TVALID,
input O_TREADY,
output [63:0] O_TDATA,
output [3:0] O_TUSER,
output O_TLAST
);

assign I_TREADY = O_TREADY;
assign O_TVALID = I_TVALID;
assign O_TDATA = I_TDATA;
assign O_TUSER = (I_TVALID) ? 8 : 0;
assign O_TLAST = I_TLAST;

endmodule
