`timescale 1ns / 1ps

// 対向アドレスを一定時間でexpireする。

module peer_addr_expire(
input CLK,
input RESETN,
input [31:0] I_PEER_ADDR,
input [63:0] I_UDP_D,
input [3:0] I_UDP_LEN,
output [31:0] O_PEER_ADDR,
output O_PEER_ADDR_EN
);

reg [31:0] o_peer_addr;
reg o_peer_addr_en;
reg [31:0] count;

assign O_PEER_ADDR = o_peer_addr;
assign O_PEER_ADDR_EN = o_peer_addr_en;

always @(posedge CLK) begin
    if (!RESETN) begin
        o_peer_addr <= 0;
        o_peer_addr_en <= 0;
    end else begin
        if (I_UDP_LEN != 0) begin
            count <= (1 << 31); // 2Gサイクルでexpireする
            o_peer_addr <= I_PEER_ADDR;
            o_peer_addr_en <= 1;
        end else if (count > 0) begin
            count <= count - 1;
        end else begin
            o_peer_addr <= 0;
            o_peer_addr_en <= 0;
        end
    end
end

endmodule
