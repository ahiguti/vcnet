
module address_mac_ip(
output [47:0] MAC_ADDR,
output [31:0] IP_ADDR,
output [15:0] IP_PORT
);
parameter IP0 = 192;
parameter IP1 = 168;
parameter IP2 = 200;
parameter IP3 = 3;
parameter PORT = 9999;

wire [7:0] ip0 = IP0;
wire [7:0] ip1 = IP1;
wire [7:0] ip2 = IP2;
wire [7:0] ip3 = IP3;
assign MAC_ADDR = { ip3, ip2, ip1, ip0, 16'h0002 };
assign IP_ADDR = { ip3, ip2, ip1, ip0 };
assign IP_PORT = { PORT[7:0], PORT[15:8] };

endmodule
