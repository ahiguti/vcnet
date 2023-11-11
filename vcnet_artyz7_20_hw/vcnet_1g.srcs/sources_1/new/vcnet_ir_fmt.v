
module vcnet_ir_fmt(
input CLK,
input RESETN,
input I_TVALID,
output I_TREADY,
input [15:0] I_TDATA,
output O_TVALID,
input O_TREADY,
output [15:0] O_TDATA,
output [1:0] O_TSTRB,
output O_TLAST
);

assign I_TREADY = O_TREADY;
assign O_TVALID = I_TVALID;
assign O_TDATA = I_TDATA;
assign O_TSTRB = (I_TDATA != 0) ? 2'b11 : 2'b00;
assign O_TLAST = (I_TDATA == 0);

endmodule
