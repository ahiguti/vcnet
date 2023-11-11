
module sync_regs
#(parameter WIDTH = 8, DEPTH = 2)
(
input CLK,
input [WIDTH-1:0] IVAL,
output [WIDTH-1:0] OVAL
);

(* ASYNC_REG = "TRUE" *) reg [WIDTH-1:0] s[0:DEPTH];
assign OVAL = s[DEPTH];

integer i;
always @(posedge CLK) begin
    s[0] <= IVAL;
    for (i = 0; i < DEPTH; i = i + 1) begin
        s[i + 1] <= s[i];
    end
end

endmodule
