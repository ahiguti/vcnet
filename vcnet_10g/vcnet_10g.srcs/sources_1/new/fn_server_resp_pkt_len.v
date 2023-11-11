
module fn_server_resp_pkt_len(
input CLK, // dummy
input [63:0] I_FIRST_WORD,
output [15:0] O_PKT_LEN
);

assign O_PKT_LEN = I_FIRST_WORD[7:0] + 2;

endmodule
