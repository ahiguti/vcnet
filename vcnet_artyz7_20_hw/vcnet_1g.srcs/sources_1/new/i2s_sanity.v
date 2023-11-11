
module i2s_sanity(
    input CLK,
    input RESETN,
    input S_TREADY,
    input S_TVALID,
    input [31:0] S_TDATA,
    output [31:0] DEBUG_INVALID_COUNT
);

reg [31:0] prev_data;
reg [31:0] invalid_count;

assign DEBUG_INVALID_COUNT = invalid_count;

always @(posedge CLK) begin
    if (!RESETN) begin
        prev_data <= 0;
        invalid_count <= 0;
    end else begin
        if (S_TREADY && S_TVALID) begin
            if (prev_data[7] == S_TDATA[7]) begin
                invalid_count <= invalid_count + 1;
            end
            prev_data <= S_TDATA;
        end
    end
end

endmodule
