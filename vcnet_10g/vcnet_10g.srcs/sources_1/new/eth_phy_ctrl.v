
module eth_phy_ctrl(
output MDC,
inout MDIO,
input RX_CLK,
input RX_CTRL,
input [3:0] RX_D,
output TX_CTRL,
output [3:0] TX_D
);

assign TX_CTRL = 1;
assign TX_D = 0;
assign MDIO = 1'bz;
assign MDC = 0;

endmodule
