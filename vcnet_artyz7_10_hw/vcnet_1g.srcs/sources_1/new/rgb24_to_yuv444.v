module rgb24_to_yuv444(
input CLK,
input I_DE, I_VSYNC, I_HSYNC, I_USER,
input [7:0] I_R, I_G, I_B,
output O_DE, O_VSYNC, O_HSYNC, O_USER, O_LAST,
output [7:0] O_Y, O_U, O_V,
output [15:0] VAL0, VAL1, VAL2, VAL3, VAL4, VAL5, VAL6, VAL7, VAL8
);

reg [3:0] reg0_ctrl;
reg [7:0] r, g, b;

always @(posedge CLK) begin
    reg0_ctrl <= { I_USER, I_HSYNC, I_VSYNC, I_DE };
    r <= I_R;
    g <= I_G;
    b <= I_B;
end

reg [3:0] reg1_ctrl;
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

reg [3:0] reg2_ctrl;
reg [15:0] y_16, u_16, v_16;

always @(posedge CLK) begin
    reg2_ctrl <= reg1_ctrl;
    y_16 <= y0 + y1 + y2;
    u_16 <= u2 - u0 - u1;
    v_16 <= v0 - v1 - v2;
end

reg [3:0] reg3_ctrl;
reg [7:0] y, u, v;

always @(posedge CLK) begin
    reg3_ctrl <= reg2_ctrl;
    y <= (y_16 + 128 +  4096) >> 8;
    u <= (u_16 + 128 + 32768) >> 8;
    v <= (v_16 + 128 + 32768) >> 8;
end

reg [3:0] reg4_ctrl;
reg [7:0] reg4_y, reg4_u, reg4_v;
reg reg4_last;

always @(posedge CLK) begin
    reg4_ctrl <= reg3_ctrl;
    reg4_y <= clamp(y, 16, 235);
    reg4_u <= clamp(u, 16, 240);
    reg4_v <= clamp(v, 16, 240);
    reg4_last <= reg3_ctrl[0] && !reg2_ctrl[0];
end

assign O_DE = reg4_ctrl[0];
assign O_VSYNC = reg4_ctrl[1];
assign O_HSYNC = reg4_ctrl[2];
assign O_USER = reg4_ctrl[3];
assign O_LAST = reg4_last;
assign O_Y = reg4_y;
assign O_U = reg4_u;
assign O_V = reg4_v;

endmodule
