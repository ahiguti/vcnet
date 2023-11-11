
// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module small_ram(
CLK, ADDRA, DINA, WEA, ADDRB, DOUTB
);
parameter ADDR_BITS = 4;
parameter DATA_BITS = 32;
parameter DOUT_REGISTER = 1;
input CLK;
input [ADDR_BITS-1:0] ADDRA;
input [DATA_BITS-1:0] DINA;
input WEA;
input [ADDR_BITS-1:0] ADDRB;
output [DATA_BITS-1:0] DOUTB;

reg [DATA_BITS-1:0] ram[0:(1<<ADDR_BITS)-1];
reg [DATA_BITS-1:0] doutb;
reg [DATA_BITS-1:0] doutb_r;

assign DOUTB = DOUT_REGISTER ? doutb_r : doutb;

always @(posedge CLK) begin
    doutb <= ram[ADDRB];
    if (WEA) begin
        ram[ADDRA] <= DINA;
    end
    doutb_r <= doutb;
end

endmodule

