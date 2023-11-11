`timescale 1ns / 1ps

// パケットにUDP, IP, ETERNETのヘッダを付ける。FCSは付けない。入力パケットの最初のワードに
// パケットの長さがセットされていなければならない。最初のワードからパケット長を返す関数が
// FN_ARG_FIRST_WORD, FN_RET_PKT_LENにセットされていなければならない。
// ethernetフレームが64byte以上になるようにpaddingを入れるのは送信側の責任。
// それ以外の点については入力パケットの形式には制限は無い。
// 長さを記した最初のワードも出力パケットに出力される(そのためこのmoduleは任意形式のUDP
// パケットを出力できるわけではない)。
// SRC_MAC_ADDRなどの各種アドレスの値は、パケットの最初のワードを読みこんだサイクルでの
// 値が採用され、そのサイクルでレジスタに取り込まれる。SRC_MAC_ADDR等は
// least significant byte firstで与えること。

module axis_udp_ip_ether(
input CLK,
input RESETN,
input I_TVALID,
output I_TREADY,
input [63:0] I_TDATA,
input [3:0] I_TUSER,   // TDATAの有効なバイト数。8以下。
input I_TLAST,
output O_TVALID,
input O_TREADY,
output [63:0] FN_ARG_FIRST_WORD,
input [15:0] FN_RET_PKT_LEN,
output [63:0] O_TDATA,
output [3:0] O_TUSER, // TDATAの有効なバイト数。8以下。
output O_TLAST,
input [47:0] SRC_MAC_ADDR,
input [31:0] SRC_IP_ADDR,
input [15:0] SRC_PORT,
input [47:0] DST_MAC_ADDR,
input [31:0] DST_IP_ADDR,
input [15:0] DST_PORT,
input DST_ENABLE
);

reg out_tvalid;
reg [63:0] out_tdata;
reg [3:0] out_tuser;
reg out_tlast;
reg [63:0] buffer_data [0:5];
reg [3:0] buffer_user [0:5];
reg [5:0] buffer_last;
reg in_finished;
reg [7:0] offset;
reg [47:0] src_mac_addr;
reg [31:0] src_ip_addr;
reg [15:0] src_port;
reg [47:0] dst_mac_addr;
reg [31:0] dst_ip_addr;
reg [15:0] dst_port;
reg dst_enable;
reg [15:0] pkt_len;
reg [15:0] ipsum;

assign FN_ARG_FIRST_WORD = I_TDATA;

assign O_TVALID = out_tvalid && dst_enable; // dst_enable==0のときはパケットをdropする
assign O_TDATA = out_tdata;
assign O_TUSER = out_tuser;
assign O_TLAST = out_tlast;
assign I_TREADY = ((O_TREADY == 1) || (out_tvalid == 0)) && (in_finished == 0);
   // 前段から吸い取るかどうか = (後段が受け取り可能 || 後段へ送らない) && 末尾送信中でない

wire [15:0] ipv4_len = pkt_len + 28;
wire [15:0] udp_len = pkt_len + 8;

integer i;

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

function [15:0] bswap32;
input [31:0] x;
begin
    bswap32 = { x[7:0], x[15:8], x[23:16], x[31:24] };
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
    ipv4_sum(16'h4500,            // version, headerlen, tos
        ipv4_sum_32lsb(16'h8011,  // ttl, udp
            SRC_IP_ADDR));

always @(posedge CLK) begin
    if (!RESETN) begin
        out_tvalid <= 0;
        out_tdata <= 0;
        out_tuser <= 0;
        out_tlast <= 0;
        offset <= 0;
        for (i = 0; i < 6; i = i + 1) begin
            buffer_data[i] <= 0;
            buffer_user[i] <= 0;
            buffer_last[i] <= 0;
        end
        in_finished <= 0;
        src_mac_addr <= 0;
        src_ip_addr <= 0;
        src_port <= 0;
        dst_mac_addr <= 0;
        dst_ip_addr <= 0;
        dst_port <= 0;
        pkt_len <= 0;
        ipsum <= 0;
    end else if ((I_TVALID && I_TREADY) || ((in_finished == 1) && O_TREADY)) begin
        // bufferに積むかどうか = 前段から吸い取った || (末尾送信中 && 後段が受け取り可能)
        if (in_finished == 0) begin
            // 前段から吸い取った
            buffer_data[0] <= I_TDATA;
            buffer_user[0] <= I_TUSER;
            buffer_last[0] <= I_TLAST;
            if (I_TLAST) begin
                // 前段から吸い取りおわり、末尾送信中にする
                in_finished <= 1;
            end
        end else begin
            // 末尾送信中
            buffer_data[0] <= 0;
            buffer_user[0] <= 0;
            buffer_last[0] <= 0;
        end
        for (i = 0; i < 5; i = i + 1) begin
            buffer_data[i + 1] <= buffer_data[i];
            buffer_user[i + 1] <= buffer_user[i];
            buffer_last[i + 1] <= buffer_last[i];
        end
        if (offset == 0) begin
            // このサイクルでSRC_MAC_ADDRなどをレジスタに取り込む
            src_mac_addr <= SRC_MAC_ADDR;
            src_ip_addr <= SRC_IP_ADDR;
            src_port <= SRC_PORT;
            dst_mac_addr <= DST_MAC_ADDR;
            dst_ip_addr <= DST_IP_ADDR;
            dst_port <= DST_PORT;
            dst_enable <= DST_ENABLE;
            pkt_len <= FN_RET_PKT_LEN; // 最初のワードがFN_ARG_FIRST_WORDにセットされたときにFN_RET_PKT_LENにパケット長が返ってくる。
            ipsum <= ipv4_sum_base;
            out_tvalid <= 1; // out_tvalid==1でもdst_enable==0ならパケットはdropされる。
            out_tdata <= { SRC_MAC_ADDR[15:0], DST_MAC_ADDR[47:0] };
            out_tuser <= 8;
            out_tlast <= 0;
            offset <= 1;
        end else if (offset == 1) begin
            ipsum <= ipv4_sum(ipsum, ipv4_len);
            out_tvalid <= 1;
            out_tdata <= { 32'h00450008, SRC_MAC_ADDR[47:16] };
            out_tuser <= 8;
            out_tlast <= 0;
            offset <= 2;
        end else if (offset == 2) begin
            ipsum <= ipv4_sum_32lsb(ipsum, dst_ip_addr);
            out_tvalid <= 1;
            out_tdata <= { 48'h118000000000, bswap(ipv4_len) };
            out_tuser <= 8;
            out_tlast <= 0;
            offset <= 3;
        end else if (offset == 3) begin
            out_tvalid <= 1;
            out_tdata <= { dst_ip_addr[15:0], src_ip_addr[31:0], (bswap(ipsum) ^ 16'hffff) };
            out_tuser <= 8;
            out_tlast <= 0;
            offset <= 4;
        end else if (offset == 4) begin
            out_tvalid <= 1;
            out_tdata <= { bswap(udp_len), dst_port, src_port, dst_ip_addr[31:16] };
            out_tuser <= 8;
            out_tlast <= 0;
            offset <= 5;
        end else if (offset == 5) begin
            out_tvalid <= 1;
            out_tdata <= { buffer_data[4][47:0], 16'h0000 };
            if (buffer_last[4] && (buffer_user[4] <= 6)) begin
                out_tuser <= buffer_user[4] + 2;
                out_tlast <= 1;
                offset <= 0;
                in_finished <= 0;
            end else begin
                out_tuser <= 8;
                out_tlast <= 0;
                offset <= 6;
            end
        end else begin
            out_tvalid <= 1;
            out_tdata <= { buffer_data[4][47:0], buffer_data[5][63:48] };
            if (buffer_last[5]) begin
                out_tuser <= buffer_user[5] - 6;
                out_tlast <= 1;
                offset <= 0;
                in_finished <= 0;
            end else if (buffer_last[4] && (buffer_user[4] <= 6)) begin
                out_tuser <= buffer_user[4] + 2;
                out_tlast <= 1;
                offset <= 0;
                in_finished <= 0;
            end else begin
                out_tuser <= 8;
                out_tlast <= 0;
            end
        end
    end else if (out_tvalid && O_TREADY) begin
        // 後段にパケットの最後のワードを送り終わったときだいたいここにくる。
        // 送信すべきデータが無くなった。
        out_tvalid <= 0;
        out_tdata <= 0;
        out_tuser <= 0;
        out_tlast <= 0;
    end
end

endmodule
