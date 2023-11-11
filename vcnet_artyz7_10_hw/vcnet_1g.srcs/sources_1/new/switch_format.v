
module switch_format(
    input CLK,
    input RESETN,
    input [1:0] FORMAT,
    input I_DE,
    input [23:0] I_DATA,
    input I_VSYNC,
    input I_HSYNC,
    output O_DE,
    output [23:0] O_DATA,
    output O_VSYNC,
    output O_HSYNC,
    input F2I_DE,
    input [23:0] F2I_DATA,
    input F2I_VSYNC,
    input F2I_HSYNC
);

reg fmt_2;
reg i_de, o_de, f2i_de;
reg [23:0] i_data, o_data, f2i_data;
reg i_vsync, o_vsync, f2i_vsync;
reg i_hsync, o_hsync, f2i_hsync;

assign O_DE = o_de;
assign O_DATA = o_data;
assign O_VSYNC = o_vsync;
assign O_HSYNC = o_hsync;

always @(posedge CLK) begin
    if (!RESETN) begin
        fmt_2 <= 0;
    end else begin
        if (!i_de) begin
            fmt_2 <= (FORMAT == 2);
        end
    end
    i_de <= I_DE;
    i_data <= I_DATA;
    i_vsync <= I_VSYNC;
    i_hsync <= I_HSYNC;
    f2i_de <= F2I_DE;
    f2i_data <= F2I_DATA;
    f2i_vsync <= F2I_VSYNC;
    f2i_hsync <= F2I_HSYNC;
    if (fmt_2) begin
        o_de <= f2i_de;
        o_data <= f2i_data;
        o_vsync <= f2i_vsync;
        o_hsync <= f2i_hsync;
    end else begin
        o_de <= i_de;
        o_data <= i_data;
        o_vsync <= i_vsync;
        o_hsync <= i_hsync;
    end
end

endmodule
