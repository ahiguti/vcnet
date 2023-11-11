
module hdmi_in_stat(
input CLK,
input RESETN,
input [31:0] COUNT_FRAME_EQ,
input [31:0] COUNT_FRAME_NE,
input RD_ADDR_EN,
input [7:0] RD_ADDR,
output RD_DATA_EN,
output [31:0] RD_DATA
);

reg rd_data_en;
reg [31:0] rd_data;

assign RD_DATA_EN = rd_data_en;
assign RD_DATA = rd_data;

always @(posedge CLK) begin
    if (!RESETN) begin
        rd_data_en <= 0;
        rd_data <= 0;
    end else begin
        if (RD_ADDR_EN) begin
            rd_data_en <= 1;
            rd_data <= { COUNT_FRAME_EQ[15:0], COUNT_FRAME_NE[15:0] };
        end else begin
            rd_data_en <= 0;
            rd_data <= 0;
        end
    end
end

endmodule
