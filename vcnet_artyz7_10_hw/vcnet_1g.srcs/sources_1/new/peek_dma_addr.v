module peek_dma_addr(
input CLK,
input RESETN,
input RD_ADDR_EN,
input [7:0] RD_ADDR,
output RD_DATA_EN,
output [31:0] RD_DATA,
input [31:0] AXI_AWADDR,
input AXI_AWREADY,
input AXI_AWVALID
);

reg [31:0] axi_awaddr;
reg axi_awready;
reg axi_awvalid;
reg rd_data_en;
reg [31:0] rd_data;

assign RD_DATA_EN = rd_data_en;
assign RD_DATA = rd_data;

always @(posedge CLK) begin
  if (!RESETN) begin
    rd_data_en <= 0;
    rd_data <= 0;
  end else begin
    axi_awaddr <= AXI_AWADDR;
    axi_awready <= AXI_AWREADY;
    axi_awvalid <= AXI_AWVALID;
    if (axi_awready && axi_awvalid) begin
        rd_data <= axi_awaddr;;
    end
    rd_data_en <= 0;
    if (RD_ADDR_EN) begin
        rd_data_en <= 1;
    end
  end
end

endmodule
