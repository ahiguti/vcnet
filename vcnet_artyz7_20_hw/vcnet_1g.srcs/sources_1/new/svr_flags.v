module svr_flags(
input CLK,
input RESETN,
input WR_EN,
input [7:0] WR_ADDR,
input [31:0] WR_DATA,
output [7:0] SVR_FLAGS
);

reg [7:0] svr_flags;

assign SVR_FLAGS = svr_flags;

always @(posedge CLK) begin
    if (!RESETN) begin
        svr_flags <= 0;
    end else begin
        if (WR_EN) begin
            case (WR_ADDR)
            2: svr_flags <= WR_DATA[7:0];
            endcase
        end
    end
end

endmodule
