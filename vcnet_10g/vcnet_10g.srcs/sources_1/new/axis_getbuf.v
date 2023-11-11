
// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module axis_getbuf(
input wire CLK,
input wire RESET_N,
input wire s_axi_AXILiteS_AWVALID,
output wire s_axi_AXILiteS_AWREADY,
input wire [7:0] s_axi_AXILiteS_AWADDR,
input wire s_axi_AXILiteS_WVALID,
output wire s_axi_AXILiteS_WREADY,
input wire [31:0] s_axi_AXILiteS_WDATA,
input wire [3:0] s_axi_AXILiteS_WSTRB,
input wire s_axi_AXILiteS_ARVALID,
output wire s_axi_AXILiteS_ARREADY,
input wire [7:0] s_axi_AXILiteS_ARADDR,
output wire s_axi_AXILiteS_RVALID,
input wire s_axi_AXILiteS_RREADY,
output wire [31:0] s_axi_AXILiteS_RDATA,
output wire [1:0] s_axi_AXILiteS_RRESP,
output wire s_axi_AXILiteS_BVALID,
input wire s_axi_AXILiteS_BREADY,
output wire [1:0] s_axi_AXILiteS_BRESP,
input wire [31:0] WR_DRAM_ADDR
);

reg [1:0] rstate;
reg [1:0] wstate;

reg wait_flip;
reg [7:0] raddr;
reg [31:0] rdata;

localparam wstate_idle = 2'd0;
localparam wstate_data = 2'd1;
localparam wstate_resp = 2'd2;
localparam rstate_idle = 2'd0;
localparam rstate_busy = 2'd1;
localparam rstate_data = 2'd2;

assign s_axi_AXILiteS_AWREADY = (wstate == wstate_idle);
assign s_axi_AXILiteS_WREADY = (wstate == wstate_data);
assign s_axi_AXILiteS_ARREADY = (rstate == rstate_idle);
assign s_axi_AXILiteS_RVALID = (rstate == rstate_data);
assign s_axi_AXILiteS_RDATA = rdata;
assign s_axi_AXILiteS_RRESP = 0;
assign s_axi_AXILiteS_BVALID = (wstate == wstate_resp);
assign s_axi_AXILiteS_BRESP = 0;

always @(posedge CLK) begin
    if (!RESET_N) begin
        wstate <= wstate_idle;
        rstate <= rstate_idle;
        rdata <= 0;
    end else begin
        if (wstate == wstate_idle && s_axi_AXILiteS_AWVALID) begin
            wstate <= wstate_data;
        end else if (wstate == wstate_data && s_axi_AXILiteS_WVALID) begin
            wstate <= wstate_resp;
        end else if (wstate == wstate_resp && s_axi_AXILiteS_BREADY) begin
            wstate <= wstate_idle;
        end
        if (rstate == rstate_idle && s_axi_AXILiteS_ARVALID) begin
            raddr <= s_axi_AXILiteS_ARADDR;
            rstate <= rstate_busy;
        end else if (rstate == rstate_busy) begin
            if (raddr == 0) begin
                if (rdata != WR_DRAM_ADDR) begin
                    rdata <= WR_DRAM_ADDR;
                    rstate <= rstate_data;
                end
            end else begin
                rdata <= WR_DRAM_ADDR;
                rstate <= rstate_data;
            end
        end else if (rstate == rstate_data && s_axi_AXILiteS_RREADY) begin
            rstate <= rstate_idle;
        end
    end
end

endmodule
