
module reg_i2s(
    input CLK,
    input RESETN,
    input I_TVALID,
    output I_TREADY,
    input [31:0] I_TDATA,
    input PULL_ODATA,
    output [31:0] ODATA
);

reg [31:0] odata;
reg odata_valid;
reg odata_valid_r;

assign I_TREADY = !odata_valid;
assign ODATA = odata; // 32'h0 if invalid

always @(posedge CLK) begin
    if (!RESETN) begin
        odata <= 0;
        odata_valid <= 0;
        odata_valid_r <= 0;
    end else begin
        odata_valid_r <= odata_valid;
        if (odata_valid) begin
            if (PULL_ODATA && odata_valid_r) begin
                // odata_r has been pulled and it was a valid data.
                //  clear odata and wait for the next data
                odata_valid <= 0;
                odata <= 0;
            end
        end else begin
            if (I_TVALID && I_TREADY) begin
                odata <= I_TDATA;
                odata_valid <= 1;
            end
        end
    end
end

endmodule
