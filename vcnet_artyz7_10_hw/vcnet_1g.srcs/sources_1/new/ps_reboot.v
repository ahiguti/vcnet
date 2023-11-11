
// reboots Zynq PS when PS_REBOOT_EN is asserted

module ps_reboot(
    input CLK,
    input RESETN,
    input PS_REBOOT_EN,
    input BLK_TREADY,
    output BLK_TVALID,
    output [31:0] BLK_ADDRESS,
    output [31:0] REP_COUNT,
    output BLK_MODE_WRITE,
    input WR_TREADY,
    output WR_TVALID,
    output [31:0] WR_TDATA,
    output RD_TREADY,
    input RD_TVALID,
    input [31:0] RD_TDATA
);

reg blk_tvalid;
reg [31:0] blk_address;
reg wdata_tvalid;
reg [31:0] wdata_tdata;

assign BLK_TVALID = blk_tvalid;
assign BLK_ADDRESS = blk_address;
assign REP_COUNT = 0;
assign BLK_MODE_WRITE = 1;
assign WR_TVALID = wdata_tvalid;
assign WR_TDATA = wdata_tdata;
assign RD_TREADY = 1;

reg [1:0] state;

always @(posedge CLK) begin
    if (!RESETN) begin
        blk_tvalid <= 0;
        blk_address <= 0;
        wdata_tvalid <= 0;
        wdata_tdata <= 0;
        state <= 0;
    end else begin
        if (state == 0) begin
            if (PS_REBOOT_EN) begin
                blk_address <= 32'hf8000008;
                blk_tvalid <= 1;
                wdata_tdata <= 32'h0000df0d;
                wdata_tvalid <= 1;
                state <= 1;
            end
        end else if (state == 1) begin
            if (BLK_TVALID && BLK_TREADY) begin
                blk_tvalid <= 0;
            end
            if (WR_TVALID && WR_TREADY) begin
                wdata_tvalid <= 0;
            end
            if (BLK_TVALID == 0 && WR_TVALID == 0) begin
                blk_address <= 32'hf8000200;
                blk_tvalid <= 1;
                wdata_tdata <= 32'h00000001;
                wdata_tvalid <= 1;
                state <= 2;
            end
        end else if (state == 2) begin
            if (BLK_TVALID && BLK_TREADY) begin
                blk_tvalid <= 0;
            end
            if (WR_TVALID && WR_TREADY) begin
                wdata_tvalid <= 0;
            end
            if (BLK_TVALID == 0 && WR_TVALID == 0) begin
                state <= 0;
            end
        end
    end
end

endmodule
