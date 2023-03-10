
module xgmii_tx_fcs(
input CLK,
input RESET,
input CE,
input [3:0] TXLEN_IN,
input [63:0] TXD_IN,
output [63:0] TXD_OUT,
output [7:0] TXC_OUT,
output [31:0] DEBUG_OUT
);

reg [63:0] txd_out;
reg [7:0] txc_out;
reg [63:0] txd_rem;
reg [7:0] txc_rem;
reg [31:0] crc;
reg [1:0] state;

wire [31:0] crc_next;

crc32_multi crc_multi_inst(crc, TXD_IN, TXLEN_IN, crc_next);

assign TXD_OUT = (state == 0 && TXLEN_IN == 8) ? 64'hd5555555555555fb : txd_out;
assign TXC_OUT = (state == 0 && TXLEN_IN == 8) ? 8'h01 : txc_out;
assign DEBUG_OUT = crc;

wire [31:0]crc_out = revert32(crc_next) ^ 32'hffffffff;

function [31:0] revert32;
input [31:0] value;
integer i;
begin
    revert32 = 0;
    for (i = 0; i < 32; i = i + 1) begin
        revert32[i] = value[31 - i];
    end
end
endfunction

function [127:0] txd_trailing;
input [31:0] crc_out;
input [63:0] txd;
input [3:0] txlen; /* txlen <= 8 */
begin
    if (txlen == 8) begin
        txd_trailing = { 32'h070707fd, crc_out[31:0], txd[63:0] };
    end else begin
        txd = txd << ((8 - txlen[2:0]) * 8);
        txd_trailing = { 96'h0707070707070707070707fd, crc_out[31:0], txd[63:0] } >> ((8 - txlen[2:0]) * 8);
    end
end
endfunction

function [15:0] txc_trailing;
input [3:0] txlen;  /* txlen <= 8 */
begin
    if (txlen == 8) begin
        txc_trailing = 16'hf000;
    end else begin
        txc_trailing = 16'hfff0 << txlen[2:0];
    end
end
endfunction

always @(posedge CLK) begin
    if (RESET) begin
        txd_out <= 64'h0707070707070707;
        txc_out <= 8'hff;
        txd_rem <= 0;
        txc_rem <= 0;
        crc <= 32'hffffffff;
        state <= 0;
    end else if (!CE) begin
        // noop
    end else begin
        if (state == 0) begin
            txd_out <= 64'h0707070707070707;
            txc_out <= 8'hff;
            crc <= 32'hffffffff;
            if (TXLEN_IN == 8) begin
                txd_out <= TXD_IN;
                txc_out <= 8'h00;
                crc <= crc_next;
                state <= 1;
            end
        end else if (state == 1) begin
            txd_out <= txd_trailing(crc_out, TXD_IN, TXLEN_IN);
            txc_out <= txc_trailing(TXLEN_IN);
            txd_rem <= txd_trailing(crc_out, TXD_IN, TXLEN_IN) >> 64;
            txc_rem <= txc_trailing(TXLEN_IN) >> 8;
            crc <= crc_next;
            if (TXLEN_IN < 8) begin
                state <= 2;
            end
        end else if (state == 2) begin
            txd_out <= txd_rem;
            txc_out <= txc_rem;
            crc <= 32'hffffffff;
            state <= 0;
        end
    end
end

endmodule
