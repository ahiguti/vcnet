module yuv444_to_yuv422(
input CLK,
input I_DE, I_VSYNC, I_HSYNC,
input [7:0] I_Y, I_U, I_V,
output O_DE, O_VSYNC, O_HSYNC,
output [7:0] O_Y, O_UV
);

reg [2:0] reg0_ctrl;
reg [23:0] reg0_yuv;

always @(posedge CLK) begin
    reg0_ctrl <= { I_DE, I_VSYNC, I_HSYNC };
    reg0_yuv <= { I_Y, I_U, I_V };
end

reg [2:0] reg1_ctrl;
reg [23:0] reg1_yuv;
reg reg0_offset, reg1_offset;

always @(posedge CLK) begin
    reg1_ctrl <= reg0_ctrl;
    reg1_yuv <= (reg0_ctrl[2]) ? reg0_yuv : reg1_ctrl[2] ? reg1_yuv : { I_DE, I_VSYNC, I_HSYNC };
    reg0_offset <= (I_DE && !reg0_ctrl[2]) ? 0 : !reg0_offset;
    reg1_offset <= reg0_offset;
end

wire [9:0] u_sum = (reg1_yuv[15:8]<<1) + reg0_yuv[15:8] + I_U + 2;
wire [9:0] v_sum = (reg1_yuv[7:0]<<1) + reg0_yuv[7:0] + I_V + 2;

reg [2:0] reg2_ctrl;
reg [23:0] reg2_yuv;
reg reg2_offset;

always @(posedge CLK) begin
    reg2_ctrl <= reg1_ctrl;
    reg2_yuv[23:16] <= reg1_yuv[23:16];
    reg2_yuv[15:8] <= (u_sum>>2);
    reg2_yuv[7:0] <= (v_sum>>2);
    reg2_offset <= reg1_offset;
end

reg o_de;
reg o_vsync;
reg o_hsync;
reg [7:0] o_y;
reg [7:0] o_uv;

always @(posedge CLK) begin
    o_de <= reg2_ctrl[2];
    o_vsync <= reg2_ctrl[1];
    o_hsync <= reg2_ctrl[0];
    o_y <= reg2_yuv[23:16];
    o_uv <= reg2_offset ? reg2_yuv[7:0] : reg2_yuv[15:8];
end

assign O_DE = o_de;
assign O_VSYNC = o_vsync;
assign O_HSYNC = o_hsync;
assign O_Y = o_y;
assign O_UV = o_uv;

endmodule
