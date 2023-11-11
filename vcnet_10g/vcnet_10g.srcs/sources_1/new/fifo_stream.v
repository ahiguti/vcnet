

// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module fifo_stream(
CLK, RESET_N, FF_CMD_PEEK, FF_CMD_POP, FF_CMD_PRST, FF_RDATA,
FF_PEEKLEN, FF_POPLEN, FF_NOPEEK, FF_NOPOP, SOUT_RDY, SOUT_VLD,
SOUT_DATA
);
parameter FF_ADDR_BITS = 8;
parameter DATA_BITS = 32;
parameter DOUT_REGISTER = 1;
input CLK;
input RESET_N;
output FF_CMD_PEEK;
output FF_CMD_POP;
output FF_CMD_PRST;
input [DATA_BITS-1:0] FF_RDATA;
input [FF_ADDR_BITS:0] FF_PEEKLEN;
input [FF_ADDR_BITS:0] FF_POPLEN;
input FF_NOPEEK;
input FF_NOPOP;
input wire SOUT_RDY;
output wire SOUT_VLD;
output wire [DATA_BITS-1:0] SOUT_DATA;

assign FF_CMD_PEEK = 0;
assign FF_CMD_POP = SOUT_RDY && SOUT_VLD;
assign FF_CMD_PRST = !FF_CMD_POP;
assign SOUT_VLD = !FF_NOPEEK;
assign SOUT_DATA = FF_RDATA;

/*
reg [1:0] state;
reg [1:0] ff_cmd;
reg [2:0] peek_valid_r;

assign FF_CMD_PEEK = ff_cmd == 2;
assign FF_CMD_POP = ff_cmd == 3;
assign FF_CMD_PRST = ff_cmd == 0;
assign SOUT_VLD = peek_valid_r[0 + DOUT_REGISTER];
assign SOUT_DATA = FF_RDATA;

always @(posedge CLK) begin
    peek_valid_r <= { peek_valid_r, (!FF_NOPEEK) && (ff_cmd != 0) };
    if (!RESET_N) begin
	   state <= 0;
	   ff_cmd <= 0;
	   peek_valid_r <= 0;
    end else begin
        if (state == 0) begin
            if (SOUT_RDY && !FF_NOPEEK) begin
                ff_cmd <= 2; // PEEK
                state <= 1;
            end
        end else if (state == 1) begin
            ff_cmd <= (SOUT_VLD && SOUT_RDY) ? 3 : 2; // POP or PEEK
            if (!SOUT_RDY) begin
                ff_cmd <= 0;
                peek_valid_r <= 0;
                state <= 0;
            end
        end
    end
end
*/

endmodule
