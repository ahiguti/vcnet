
module gmii_to_xgmil(
input CLK,
input RESETN,
input RXC_IN,
input [7:0] RXD_IN,
output [3:0] RXLEN_OUT,
output [63:0] RXD_OUT,
output CE_OUT
);

reg [3:0] rxlen_in;
reg [63:0] rxd_in;
reg [2:0] ioffset;
reg state;
reg [3:0] rxlen_out;
reg [63:0] rxd_out;
reg [2:0] offset;
reg tail_suppress;

assign RXLEN_OUT = ((offset == 0) && (rxlen_out == 8 || !tail_suppress)) ? rxlen_out : 0;
assign RXD_OUT = rxd_out;
assign CE_OUT = (offset == 0);

always @(posedge CLK) begin
    if (!RESETN) begin
        rxlen_in <= 0;
        rxd_in <= 0;
        ioffset <= 0;
        rxlen_out <= 0;
        rxd_out <= 0;
        offset <= 0;
        tail_suppress <= 0;
        state <= 0;
    end else begin
        offset <= offset + 1; // mod 8
        if (offset == 0) begin
            tail_suppress <= (rxlen_out != 8);
        end
        if (state == 0) begin
            ioffset <= 0;
            if (RXC_IN) begin
                rxlen_in <= 1;
                rxd_in <= { RXD_IN[7:0], 56'h0 };
                ioffset <= 1;
                state <= 1;
            end
        end else if (state == 1) begin
            rxlen_in <= rxlen_in == 8 ? 1 : rxlen_in + 1;
            rxd_in <= { RXD_IN[7:0], rxd_in[63:8] };
            ioffset <= ioffset + 1; // mod 8
            if (RXC_IN == 0) begin
                rxlen_in <= rxlen_in;
                rxd_in <= { 8'h0, rxd_in[63:8] };
            end
            if (ioffset == 0) begin
                rxlen_out <= rxlen_in;
                rxd_out <= rxd_in;
                if (rxlen_in != 8) begin
                    rxlen_in <= 0;
                    rxd_in <= 0;
                    state <= 0;
                end
            end
        end
    end
end

endmodule
