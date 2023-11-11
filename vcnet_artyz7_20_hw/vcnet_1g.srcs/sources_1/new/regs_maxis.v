
module regs_maxis
#(parameter ADDR = 0)
(
input CLK,
input RESETN,
input WR_EN,
input [7:0] WR_ADDR,
input [31:0] WR_DATA,
input RD_ADDR_EN,
input [7:0] RD_ADDR,
output RD_DATA_EN,
output [31:0] RD_DATA,
output O_TVALID,
input O_TREADY,
output [31:0] O_TDATA
);

reg o_tvalid;
reg [31:0] o_tdata;
reg rd_data_en;

assign O_TVALID = o_tvalid;
assign O_TDATA = o_tdata;
assign RD_DATA = !o_tvalid; // returns 0 if axis port is busy
assign RD_DATA_EN = rd_data_en;

always @(posedge CLK) begin
    if (!RESETN) begin
        o_tvalid <= 0;
        o_tdata <= 0;
        rd_data_en <= 0;
    end else begin
        if (O_TVALID && O_TREADY) begin
            o_tvalid <= 0;
        end
        if (WR_EN && (WR_ADDR == ADDR)) begin
            o_tvalid <= 1;
            o_tdata <= WR_DATA;
        end
        if (RD_ADDR_EN && (RD_ADDR == ADDR)) begin
            rd_data_en <= RD_ADDR_EN;
        end else begin
            rd_data_en <= 0;
        end
    end
end

endmodule
