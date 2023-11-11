
// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module stream_fifo(
SIN_RDY, SIN_VLD, SIN_DATA, FF_CMD_PUSH, FF_WDATA, FF_NOPUSH, FF_PUSHLEN
);
parameter FF_ADDR_BITS = 17;
parameter DATA_BITS = 32;
output wire SIN_RDY;
input wire SIN_VLD;
input wire [DATA_BITS-1:0] SIN_DATA;
output wire FF_CMD_PUSH;
output wire [DATA_BITS-1:0] FF_WDATA;
input wire FF_NOPUSH;
input wire [FF_ADDR_BITS:0] FF_PUSHLEN;

assign SIN_RDY = !FF_NOPUSH;
assign FF_CMD_PUSH = SIN_VLD;
assign FF_WDATA = SIN_DATA;

endmodule

