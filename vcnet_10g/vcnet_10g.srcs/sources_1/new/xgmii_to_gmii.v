
module xgmii_to_gmii(
input CLK,
input RESETN,
input CE,
input [63:0] TXD_IN,
input [3:0] TXLEN_IN,
output [7:0] TX_D_OUT,
output TX_CTRL_OUT
);

reg [63:0] txd;
reg [3:0] txlen;

assign TX_D_OUT = txd[7:0];
assign TX_CTRL_OUT = (txlen > 0);

always @(posedge CLK) begin
    if (!RESETN) begin
        txd <= 0;
        txlen <= 0;
    end else begin
        txd <= { 8'h0, txd[63:8] };
        txlen <= txlen > 0 ? txlen - 1 : 0;
        if (CE) begin
            txd <= TXD_IN;
            txlen <= TXLEN_IN;
        end
    end
end

endmodule
