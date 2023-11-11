module yuv411_add_dummy_pixels(
    input CLK,
    input RESETN,
    input I_DE,
    input I_VALID,
    input I_VSYNC,
    input I_HSYNC,
    input [7:0] I_Y0,
    input [7:0] I_Y1,
    input [7:0] I_UV,
    output O_DE,
    output O_VSYNC,
    output O_HSYNC,
    output [23:0] O_DATA
);

reg i_de, i_valid;
reg [2:0] i_ctrl, i_ctrl_r1, o_ctrl;
reg [23:0] i_data, i_data_r1, o_data;
reg [10:0] cwoffset, roffset, dwoffset;
reg [23:0] data[0:2047];
reg [2:0] ctrl[0:2047];

assign O_DE = o_ctrl[0];
assign O_VSYNC = o_ctrl[1];
assign O_HSYNC = o_ctrl[2];
assign O_DATA = o_data;

always @(posedge CLK) begin
    if (!RESETN) begin
        cwoffset <= 0;
        dwoffset <= 0;
        roffset <= 8;
    end else begin
        i_de <= I_DE;
        i_valid <= I_VALID;
        i_ctrl <= { I_HSYNC, I_VSYNC, I_DE };
        i_data <= { I_Y1, I_Y0, I_UV };
        cwoffset <= cwoffset + 1;
        if (!i_de) begin
            dwoffset <= cwoffset + 1;
            i_data_r1 <= 0;
        end else if (i_valid) begin
            dwoffset <= dwoffset + 1;
            i_data_r1 <= i_data;
        end
        roffset <= roffset + 1;
        i_ctrl_r1 <= i_ctrl;
        ctrl[cwoffset] <= i_ctrl_r1;
        data[dwoffset] <= i_data_r1;
        o_ctrl <= ctrl[roffset];
        o_data <= data[roffset];
    end
end

endmodule
