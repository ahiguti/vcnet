`timescale 1ns / 1ps

module xgmii_to_axis(
input CLK,
input [63:0] I_DATA,
input [3:0] I_LEN,
output O_TVALID,
output [63:0] O_TDATA,
output [3:0] O_TUSER,
output O_TLAST
);

reg [63:0] i_data;
reg [3:0] i_len;
reg [63:0] o_tdata;
reg o_tvalid;
reg [3:0] o_tuser;
reg o_tlast;

assign O_TVALID = o_tvalid;
assign O_TDATA = o_tdata;
assign O_TUSER = o_tuser;
assign O_TLAST = o_tlast;

always @(posedge CLK) begin
    i_data <= I_DATA;
    i_len <= I_LEN;
    o_tvalid <= (i_len != 0);
    o_tdata <= i_data;
    o_tuser <= i_len;
    o_tlast <= ((i_len != 8 && i_len != 0) || (i_len == 8 && I_LEN == 0));
end

endmodule
