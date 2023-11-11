module add_dummy_pixels(
    input CLK,
    input I_VALID,
    input [23:0] I_DATA,
    input I_VSYNC,
    input I_HSYNC,
    output O_VALID,
    output [23:0] O_DATA,
    output O_VSYNC,
    output O_HSYNC
);

reg [11:0] num_pixels;
reg valid;
reg [23:0] data;
reg vsync, hsync;

assign O_VALID = valid;
assign O_DATA = data;
assign O_VSYNC = vsync;
assign O_HSYNC = hsync;

always @(posedge CLK) begin
    valid <= I_VALID;
    data <= I_DATA;
    vsync <= I_VSYNC;
    hsync <= I_HSYNC;
    if (I_VALID) begin
        num_pixels <= num_pixels + 1;
    end else if (num_pixels != 0) begin
        num_pixels <= num_pixels - 1;
        valid <= 1;
        data <= 0;
    end
end

endmodule
