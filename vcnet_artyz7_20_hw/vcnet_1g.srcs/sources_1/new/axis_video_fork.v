
module axis_video_fork(
input CLK,
input RESETN,
input S_TVALID,
output S_TREADY,
input [23:0] S_TDATA,
input S_TLAST,
input S_TUSER,
output M0_TVALID,
input M0_TREADY,
output [23:0] M0_TDATA,
output [2:0] M0_TKEEP,
output M0_TLAST,
output M0_TUSER,
output M1_TVALID,
input M1_TREADY,
output [23:0] M1_TDATA,
output [2:0] M1_TKEEP,
output M1_TLAST,
output M1_TUSER
);

assign S_TREADY = M0_TREADY;
assign M0_TVALID = S_TVALID;
assign M1_TVALID = S_TVALID;
assign M0_TDATA = S_TDATA;
assign M1_TDATA = S_TDATA;
assign M0_TKEEP = 7;
assign M1_TKEEP = 7;
assign M0_TLAST = S_TLAST;
assign M1_TLAST = S_TLAST;
assign M0_TUSER = S_TUSER;
assign M1_TUSER = S_TUSER;

endmodule
