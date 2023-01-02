
module ten_gig_eth_test(
input default_sysclk_250_clk_n,
input default_sysclk_250_clk_p,
input reset,
output [7:0] LED,
input MGTCLK0_clk_n,
input MGTCLK0_clk_p,
input SFP1_RX_LOS,
input SFP1_RX_N,
input SFP1_RX_P,
output SFP1_TX_DISABLE,
output SFP1_TX_N,
output SFP1_TX_P,
input SFP2_RX_LOS,
input SFP2_RX_N,
input SFP2_RX_P,
output SFP2_TX_DISABLE,
output SFP2_TX_N,
output SFP2_TX_P,
output PHY1_GTX_CLK,
output PHY1_MDC,
inout PHY1_MDIO,
output [0:0] PHY1_RESETN,
input PHY1_RX_CLK,
input PHY1_RX_CTRL,
input [3:0] PHY1_RX_D,
output PHY1_TX_CTRL,
output [3:0] PHY1_TX_D);

wire sysclk_125;
wire sysclk_locked;

design_eth10g design_eth10g_i
       (.LED(LED[0:0]),
        .MGTCLK0_clk_n(MGTCLK0_clk_n),
        .MGTCLK0_clk_p(MGTCLK0_clk_p),
        .SFP1_RX_LOS(SFP1_RX_LOS),
        .SFP1_RX_N(SFP1_RX_N),
        .SFP1_RX_P(SFP1_RX_P),
        .SFP1_TX_DISABLE(SFP1_TX_DISABLE),
        .SFP1_TX_N(SFP1_TX_N),
        .SFP1_TX_P(SFP1_TX_P),
        .SFP2_RX_LOS(SFP2_RX_LOS),
        .SFP2_RX_N(SFP2_RX_N),
        .SFP2_RX_P(SFP2_RX_P),
        .SFP2_TX_DISABLE(SFP2_TX_DISABLE),
        .SFP2_TX_N(SFP2_TX_N),
        .SFP2_TX_P(SFP2_TX_P),
        .sysclk_125(sysclk_125),
        .reset(reset));
bd_sysclk bd_sysclk_i
       (.default_sysclk_250_clk_n(default_sysclk_250_clk_n),
        .default_sysclk_250_clk_p(default_sysclk_250_clk_p),
        .reset(reset),
        .sysclk_125(sysclk_125),
        .sysclk_locked(sysclk_locked));
design_gmii_test design_gmii_test_i
       (.DEBUG_CLK(LED[1:1]),
        .PHY1_GTX_CLK(PHY1_GTX_CLK),
        .PHY1_MDC(PHY1_MDC),
        .PHY1_MDIO(PHY1_MDIO),
        .PHY1_RESETN(PHY1_RESETN),
        .PHY1_RX_CLK(PHY1_RX_CLK),
        .PHY1_RX_CTRL(PHY1_RX_CTRL),
        .PHY1_RX_D(PHY1_RX_D),
        .PHY1_TX_CTRL(PHY1_TX_CTRL),
        .PHY1_TX_D(PHY1_TX_D),
        .sysclk_125(sysclk_125),
        .sysclk_locked(sysclk_locked),
        .reset(reset));
endmodule
