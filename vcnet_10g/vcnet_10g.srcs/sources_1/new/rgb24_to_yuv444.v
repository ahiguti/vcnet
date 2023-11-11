module rgb24_to_yuv444(
input CLK,
input I_DE, I_VSYNC, I_HSYNC,
input [7:0] I_R, I_G, I_B,
output O_DE, O_VSYNC, O_HSYNC,
output [7:0] O_Y, O_U, O_V,
output [15:0] VAL0, VAL1, VAL2, VAL3, VAL4, VAL5, VAL6, VAL7, VAL8
);

reg [2:0] reg0_ctrl;
reg [7:0] r, g, b;

always @(posedge CLK) begin
    reg0_ctrl <= { I_DE, I_VSYNC, I_HSYNC };
    r <= I_R;
    g <= I_G;
    b <= I_B;
end

reg [2:0] reg1_ctrl;
reg [15:0] y0, y1, y2, u0, u1, u2, v0, v1, v2;

always @(posedge CLK) begin
    reg1_ctrl <= reg0_ctrl;
    y0 <= (r<<6) + (r<<1);          // 66
    y1 <= (g<<7) + (g<<0);          // 128
    y2 <= (b<<4) + (b<<3) + (b<<0); // 25
    u0 <= (r<<5) + (r<<2) + (r<<1); // 38
    u1 <= (g<<6) + (g<<3) + (g<<1); // 74
    u2 <= (b<<6) + (b<<5) + (b<<4); // 112
    v0 <= (r<<6) + (r<<5) + (r<<4); // 112
    v1 <= (g<<6) + (g<<5) - (g<<1); // 94
    v2 <= (b<<4) + (b<<1);          // 18
end

assign VAL0 = y0;
assign VAL1 = y1;
assign VAL2 = y2;
assign VAL3 = u0;
assign VAL4 = u1;
assign VAL5 = u2;
assign VAL6 = v0;
assign VAL7 = v1;
assign VAL8 = v2;

function [7:0] clamp(input [7:0] x, mi, mx);
    clamp = x < mi ? mi : x > mx ? mx : x;
endfunction

wire [15:0] y_16 = 128 +  4096 + y0 + y1 + y2;
wire [15:0] u_16 = 128 + 32768 - u0 - u1 + u2;
wire [15:0] v_16 = 128 + 32768 + v0 - v1 - v2;

reg [2:0] reg2_ctrl;
reg [7:0] y, u, v;

always @(posedge CLK) begin
    reg2_ctrl <= reg1_ctrl;
    y <= y_16 >> 8;
    u <= u_16 >> 8;
    v <= v_16 >> 8;
end

assign O_DE = reg2_ctrl[2];
assign O_VSYNC = reg2_ctrl[1];
assign O_HSYNC = reg2_ctrl[0];
assign O_Y = clamp(y, 16, 235);
assign O_U = clamp(u, 16, 240);
assign O_V = clamp(v, 16, 240);

endmodule
