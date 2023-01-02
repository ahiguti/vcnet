
module axis_block(
    CLK, RESETN, FIFO_PROG_EMPTY, I_TREADY, I_TVALID, I_TDATA, O_TREADY, O_TVALID, O_TDATA
);
parameter WIDTH_DATA = 32;
parameter BLOCK_SIZE = 256;
input CLK;
input RESETN;
input FIFO_PROG_EMPTY;
output I_TREADY;
input I_TVALID;
input [WIDTH_DATA-1:0] I_TDATA;
input O_TREADY;
output O_TVALID;
output [WIDTH_DATA-1:0] O_TDATA;

reg [16:0] down_remain;

assign I_TREADY = O_TREADY && (down_remain != 0);
assign O_TVALID = I_TVALID && (down_remain != 0);
assign O_TDATA = I_TDATA;
wire down_handshake = I_TVALID && O_TREADY && (down_remain != 0);

always @(posedge CLK) begin
    if (!RESETN) begin
        down_remain <= 0;
    end else begin
        if (!FIFO_PROG_EMPTY && down_remain == 0) begin
            down_remain <= BLOCK_SIZE;
        end else begin
            down_remain <= down_remain - down_handshake;
        end
    end
end

endmodule
