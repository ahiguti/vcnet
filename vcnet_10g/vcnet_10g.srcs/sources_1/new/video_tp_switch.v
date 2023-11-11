
// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module video_tp_switch(
input CLK,
input RESETN,
//input SW,
input [23:0] I0_TDATA,
input I0_TLAST,
input I0_TUSER,
input I0_TVALID,
output I0_TREADY,
input [23:0] I1_TDATA,
input I1_TLAST,
input I1_TUSER,
input I1_TVALID,
output I1_TREADY,
output [23:0] O_TDATA,
output O_TLAST,
output O_TUSER,
output O_TVALID,
input O_TREADY
);

//reg [2:0] sw_reg;
wire sw = 0; // SW; // sw_reg[2];
//assign O_TDATA = I0_TDATA;
//assign O_TVALID = I0_TVALID;
//assign O_TLAST = I0_TLAST;
//assign O_TUSER = I0_TUSER;
//assign I0_TREADY = O_TREADY;
//assign I1_TREADY = 0;
assign O_TDATA = sw ? I1_TDATA : I0_TDATA;
assign O_TVALID = sw ? I1_TVALID : I0_TVALID;
assign O_TLAST = sw ? I1_TLAST : I0_TLAST;
assign O_TUSER = sw ? I1_TUSER : I0_TUSER;
assign I0_TREADY = sw ? 0 : O_TREADY;
assign I1_TREADY = sw ? O_TREADY : 0;

//always @(posedge CLK) begin
//    if (!RESETN) begin
//        sw_reg <= 0;
//    end else begin
//        sw_reg <= { sw_reg[1:0], SW };
//    end
//end

endmodule
