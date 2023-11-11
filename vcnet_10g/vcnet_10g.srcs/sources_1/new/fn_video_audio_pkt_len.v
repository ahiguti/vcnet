
module fn_video_audio_pkt_len(
input CLK,
input [63:0] I_FIRST_WORD,
output [15:0] O_PKT_LEN
);

assign O_PKT_LEN = I_FIRST_WORD[15:0];

endmodule
