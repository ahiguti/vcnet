
// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module fifo_reg(
CLK, RESET_N, CMD_PUSH, CMD_PEEK, CMD_POP, CMD_PRST, WDATA, RDATA,
CUR_PUSHLEN, CUR_PEEKLEN, CUR_POPLEN, CUR_NOPUSH, CUR_NOPEEK, CUR_NOPOP
);
parameter ADDR_BITS = 4;
parameter DATA_BITS = 32;
input CLK;
input RESET_N;
input CMD_PUSH; // push entry to the queue
input CMD_PEEK; // increment peek_offset without disposing 
input CMD_POP; // dispose the oldest entry
input CMD_PRST; // reset peek_offset to the oldest entry
input [DATA_BITS-1:0] WDATA;
  // data to push. valid when CMD_PUSH_C is asserted
output [DATA_BITS-1:0] RDATA;
  // data peeked, valid when CMD_PEEK_C is asserted in the prev cycle
output [ADDR_BITS:0] CUR_PUSHLEN;
output [ADDR_BITS:0] CUR_PEEKLEN;
output [ADDR_BITS:0] CUR_POPLEN;
output CUR_NOPUSH; // no space to push
output CUR_NOPEEK; // no data to peek
output CUR_NOPOP; // no data to pop
wire [ADDR_BITS-1:0] br_addra;
wire [DATA_BITS-1:0] br_dina;
wire [0:0] br_wea;
wire [ADDR_BITS-1:0] br_addrb;
wire [DATA_BITS-1:0] br_doutb;

reg [ADDR_BITS:0] cur_poplen;
  // becomes 2^ADDR_BITS when queue is full to pop
reg [ADDR_BITS:0] cur_peeklen;
  // becomes 2^ADDR_BITS when queue is full to peek
reg [ADDR_BITS-1:0] cur_push_offset; // where to push next
reg [ADDR_BITS-1:0] cur_pop_offset; // where to pop next
reg [ADDR_BITS-1:0] cur_peek_offset; // where to peek next

wire CMD_PUSH_C = CMD_PUSH && (!CUR_NOPUSH);
wire CMD_PEEK_C = (CMD_PEEK || CMD_POP) && (!CMD_PRST) && (!CUR_NOPEEK);
  // CMD_PEEK is ignored if CMD_PRST is set
wire CMD_POP_C = CMD_POP && (!CUR_NOPOP);

assign RDATA = br_doutb;
assign CUR_PUSHLEN = (1 << ADDR_BITS) - cur_poplen;
assign CUR_PEEKLEN = cur_peeklen;
assign CUR_POPLEN = cur_poplen;
assign CUR_NOPUSH = cur_poplen[ADDR_BITS]; // equiv to CUR_PUSHLEN == 0
assign CUR_NOPEEK = cur_peeklen == 0;
assign CUR_NOPOP = cur_poplen == 0;
assign br_addra = cur_push_offset;
assign br_dina = WDATA;
assign br_wea = CMD_PUSH_C;
assign br_addrb = cur_peek_offset;

reg_ram
  #(.ADDR_BITS(ADDR_BITS), .DATA_BITS(DATA_BITS))
rrm(
    .CLK(CLK), .ADDRA(br_addra), .DINA(br_dina), .WEA(br_wea),
    .ADDRB(br_addrb), .DOUTB(br_doutb));

always @(posedge CLK) begin
    if (!RESET_N) begin
        cur_peeklen <= 0;
        cur_poplen <= 0;
        cur_push_offset <= 0;
        cur_peek_offset <= 0;
        cur_pop_offset <= 0;
    end else begin
        cur_push_offset <= cur_push_offset + CMD_PUSH_C;
        cur_peek_offset <= cur_peek_offset + CMD_PEEK_C;
        cur_pop_offset <= cur_pop_offset + CMD_POP_C;
        cur_peeklen <= cur_peeklen + CMD_PUSH_C - CMD_PEEK_C;
        cur_poplen <= cur_poplen + CMD_PUSH_C - CMD_POP_C;
        if (CMD_PRST) begin
            cur_peek_offset <= cur_pop_offset + CMD_POP_C;
	      // reset to cur_pop_offset
            cur_peeklen <= cur_poplen + CMD_PUSH_C - CMD_POP_C;
	      // reset to cur_poplen
        end
    end
end

endmodule
