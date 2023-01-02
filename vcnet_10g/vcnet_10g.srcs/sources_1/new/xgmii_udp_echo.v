
module xgmii_udp_echo(
input CLK,
input RESET,
input [47:0] LOCAL_MAC,
input [31:0] LOCAL_IP,
input [15:0] LOCAL_PORT,
input CE,
input [3:0] RXLEN_IN,
input [63:0] RXD_IN,
output [3:0] UDP_LEN_OUT,
output [63:0] UDP_D_OUT,
output [3:0] TXLEN_OUT,
output [63:0] TXD_OUT,
output [47:0] PEER_MAC_ADDR,
output [31:0] PEER_IP_ADDR,
output [15:0] PEER_PORT,
output [15:0] DEBUG_OUT
);

// 22:00:00:00:00:01, 192.168.200.3
// localparam LOCAL_MAC = 48'h010000000022;
// localparam LOCAL_IP = { 8'd3, 8'd200, 8'd168, 8'd192 };

reg [15:0] rxoffset;
reg [47:0] rx_src_mac;
reg [47:0] rx_dest_mac;
reg rx_is_ipv4;
reg rx_is_udp;
reg [31:0] rx_src_ip;
reg [31:0] rx_dest_ip;
reg [15:0] rx_src_port;
reg [15:0] rx_dest_port;
reg [15:0] rx_iplen;
reg [15:0] txoffset;
reg [63:0] txd;
reg [3:0] txlen;
reg [64*5-1:0] rxd_shiftreg;
reg [4*5-1:0] rxlen_shiftreg;
reg [15:0] tx_ipv4_sum;
reg [47:0] peer_mac_addr;
reg [31:0] peer_ip_addr;
reg [15:0] peer_port;
reg [47:0] udp_d_prev;
reg [3:0] udp_len_prev;
reg [63:0] udp_d;
reg [3:0] udp_len;

wire [63:0] rxd_copy = rxd_shiftreg[64*3+63:64*3];
wire [3:0] rxlen_copy = rxlen_shiftreg[4*3+3:4*3];
//wire [63:0] rxd_copy_1 = rxd_shiftreg[64*2+63:64*2];
//wire [3:0] rxlen_copy_1 = rxlen_shiftreg[4*2+3:4*2];

function [15:0] ipv4_sum;
input [15:0] x;
input [15:0] y;
reg [16:0] z;
begin
    z = x + y;
    ipv4_sum = z[15:0] + z[16];
end
endfunction

function [15:0] bswap;
input [15:0] x;
begin
    bswap = { x[7:0], x[15:8] };
end
endfunction

function [15:0] ipv4_sum_32lsb;
input [15:0] x;
input [31:0] y;
begin
    ipv4_sum_32lsb = ipv4_sum(x, ipv4_sum(bswap(y[15:0]), bswap(y[31:16])));
end
endfunction

wire [15:0] ipv4_sum_base =
    ipv4_sum(16'h4500, // version, headerlen, tos
    ipv4_sum_32lsb(16'h8011, // ttl, udp
        LOCAL_IP));

assign TXD_OUT = txd;
assign TXLEN_OUT = txlen;
assign PEER_MAC_ADDR = peer_mac_addr;
assign PEER_IP_ADDR = peer_ip_addr;
assign PEER_PORT = peer_port;
assign UDP_D_OUT = udp_d;
assign UDP_LEN_OUT = udp_len;
assign DEBUG_OUT = tx_ipv4_sum;

always @(posedge CLK) begin
    if (RESET) begin
        rxoffset <= 0;
        txoffset <= 0;
        txd <= 0;
        txlen <= 0;
        peer_mac_addr <= 0;
        peer_ip_addr <= 0;
        peer_port <= 0;
        udp_d_prev <= 0;
        udp_len_prev <= 0;
        udp_d <= 0;
        udp_len <= 0;
    end else if (!CE) begin
        // do nothing
    end else begin
        rxd_shiftreg <= { rxd_shiftreg, RXD_IN };
        rxlen_shiftreg <= { rxlen_shiftreg, RXLEN_IN };
        rxoffset <= RXLEN_IN == 8 ? rxoffset + 1 : 0;
        if (rxoffset == 0) begin
            rx_src_mac[15:0] <= RXD_IN[63:48];
        end else if (rxoffset == 1) begin
            rx_src_mac[47:16] <= RXD_IN[31:0];
            rx_is_ipv4 <= (RXD_IN[63:32] == 32'h00450008);
        end else if (rxoffset == 2) begin
            rx_iplen <= RXD_IN[15:0];
            rx_is_udp <= (RXD_IN[63:56] == 8'h11);
        end else if (rxoffset == 3) begin
            rx_src_ip <= RXD_IN[47:16];
            rx_dest_ip[15:0] <= RXD_IN[63:48];
        end else if (rxoffset == 4) begin
            rx_dest_ip[23:16] <= RXD_IN[15:0];
            rx_src_port <= RXD_IN[31:16];
            rx_dest_port <= RXD_IN[47:32];
        end else if (rxoffset == 5) begin
        end else begin
            if (RXLEN_IN != 8) begin
                rxoffset <= 0;
            end
        end
        txoffset <= txoffset + 1;
        if (txoffset == 0) begin
            txd <= 0;
            txlen <= 0;
            txoffset <= 0;
            if (rxoffset == 4 && rx_is_ipv4 && rx_is_udp &&
                RXD_IN[47:32] == LOCAL_PORT) begin // ipv4 udp port
                peer_mac_addr <= rx_src_mac;
                peer_ip_addr <= rx_src_ip;
                peer_port <= RXD_IN[31:16];
                txd <= { LOCAL_MAC[15:0], rx_src_mac[47:0] };
                txlen <= rxlen_copy;
                txoffset <= 1;
                tx_ipv4_sum <= ipv4_sum_32lsb(ipv4_sum_base, rx_src_ip);
            end
        end else if (txoffset == 1) begin
            txd <= { 32'h00450008, LOCAL_MAC[47:16] };
            txlen <= rxlen_copy;
        end else if (txoffset == 2) begin
            tx_ipv4_sum <= ipv4_sum(tx_ipv4_sum, bswap(rx_iplen));
            txd <= { rxd_copy[63:32], 16'h0000, rx_iplen };
            txlen <= rxlen_copy;
        end else if (txoffset == 3) begin
            txd <= { rx_src_ip[15:0], LOCAL_IP[31:0],
                bswap(tx_ipv4_sum) ^ 16'hffff };
            txlen <= rxlen_copy;
        end else if (txoffset == 4) begin
            txd <= { rxd_copy[63:48], rx_src_port[15:0], rx_dest_port[15:0],
                rx_src_ip[31:16] };
            txlen <= rxlen_copy;
        end else if (txoffset == 5) begin
            txd <= { (rxd_copy[63:16] ^ 48'h0), 16'h0000 };
            txlen <= rxlen_copy;
            if (rxlen_copy != 8) begin
                txoffset <= 0;
            end
        end else begin
            txd <= rxd_copy ^ 64'h0;
            txlen <= rxlen_copy;
            if (rxlen_copy != 8) begin
                txoffset <= 0;
            end
        end
        if (txoffset == 5) begin
            udp_len_prev <= (rxlen_copy > 2) ? (rxlen_copy - 2) : 0;
            udp_d_prev <= (rxlen_copy > 2) ? rxd_copy[63:16] : 0;
            udp_len <= 0;
            udp_d <= 0;
        end else if (txoffset >= 6) begin
            udp_len_prev <= (rxlen_copy > 2) ? (rxlen_copy - 2) : 0;
            udp_d_prev <= (rxlen_copy > 2) ? rxd_copy[63:16] : 0;
            udp_len <= (rxlen_copy > 2) ? 8 : (rxlen_copy + 6);
            udp_d <= { rxd_copy[15:0], udp_d_prev[47:0] };
        end else begin
            udp_len_prev <= 0;
            udp_d_prev <= 0;
            udp_len <= udp_len_prev;
            udp_d <= { 16'h0000, udp_d_prev[47:0] };
        end
    end
end

endmodule

