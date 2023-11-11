
module sync_regs
#(parameter WIDTH = 8, DEPTH = 2)
(
input CLK,
input [WIDTH-1:0] IVAL,
output [WIDTH-1:0] OVAL
);

(* ASYNC_REG = "TRUE" *) reg [WIDTH-1:0] metastability_guard[0:DEPTH];
// set_false_path -to [get_cells -hier *metastability_guard*]

assign OVAL = metastability_guard[DEPTH];

integer i;
always @(posedge CLK) begin
    metastability_guard[0] <= IVAL;
    for (i = 0; i < DEPTH; i = i + 1) begin
        metastability_guard[i + 1] <= metastability_guard[i];
    end
end

endmodule
