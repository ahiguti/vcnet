module fifo_read_throttle(
input CLK,
input RESET,
input EMPTY_IN,
input VALID_IN,
output READ_EN_OUT,
input [7:0] DATA_IN,
output CTRL_OUT,
output [7:0] DATA_OUT,
output [3:0] DEBUG_OUT
);

reg read_en;
reg [3:0] cnt;

assign READ_EN_OUT = read_en;
assign CTRL_OUT = VALID_IN;
assign DATA_OUT = VALID_IN ? DATA_IN[7:0] : 0;
assign DEBUG_OUT = cnt;

always @(posedge CLK) begin
    if (RESET) begin
        read_en <= 0;
        cnt <= 0;
    end else begin
        if (cnt < 3) begin
            if (!EMPTY_IN) begin
                cnt <= cnt + 1;
            end else begin
                cnt <= 0;
            end
        end else if (cnt < 5) begin
            read_en <= 1;
            cnt <= cnt + 1;
        end else if (cnt == 5) begin
            if (!VALID_IN) begin
                read_en <= 0;
                cnt <= 0;
            end
        end
    end
end

endmodule
