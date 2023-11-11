module rgb24_to_yuv444_fullswing(
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
reg [15:0] yr, yg, yb, ur, ug, ub, vr, vg, vb;

always @(posedge CLK) begin
    reg1_ctrl <= reg0_ctrl;
    yr <= (r<<0) + (r<<2) + (r<<3) + (r<<6); // 77
    yg <= (g<<1) + (g<<2) + (g<<4) + (g<<7); // 150
    yb <= (b<<0) + (b<<2) + (b<<3) + (b<<4); // 29
    ur <= (r<<0) + (r<<1) + (r<<3) + (r<<5); // 43
    ug <= (g<<2) + (g<<4) + (g<<6);          // 84
    ub <= (b<<7);                            // 128
    vr <= (r<<7) - (r<<0);                   // 127
    vg <= (g<<1) + (g<<3) + (g<<5) + (g<<6); // 106
    vb <= (b<<0) + (b<<2) + (b<<4);          // 21
end

assign VAL0 = yr;
assign VAL1 = yg;
assign VAL2 = yb;
assign VAL3 = ur;
assign VAL4 = ug;
assign VAL5 = ub;
assign VAL6 = vr;
assign VAL7 = vg;
assign VAL8 = vb;

function [7:0] clamp(input [7:0] x, mi, mx);
    clamp = x < mi ? mi : x > mx ? mx : x;
endfunction

reg [3:0] reg2_ctrl;
reg [15:0] y_16, u_16, v_16;

always @(posedge CLK) begin
    reg2_ctrl <= reg1_ctrl;
    y_16 <= yr + yg + yb;
    u_16 <= ub - ur - ug;
    v_16 <= vr - vg - vb;
end

reg [3:0] reg3_ctrl;
reg [7:0] y, u, v;

always @(posedge CLK) begin
    reg3_ctrl <= reg2_ctrl;
    y <= (y_16 + 128        ) >> 8;
    u <= (u_16 + 127 + 32768) >> 8; // RGB=0000ff‚Ì‚Æ‚«ˆì‚ê‚È‚¢‚æ‚¤+127‚É‚·‚é
    v <= (v_16 + 128 + 32768) >> 8;
end

reg [3:0] reg4_ctrl;
reg [7:0] reg4_y, reg4_u, reg4_v;
reg reg4_last;

always @(posedge CLK) begin
    reg4_ctrl <= reg3_ctrl;
    reg4_y <= y; // clamp(y, 16, 235);
    reg4_u <= u; // clamp(u, 16, 240);
    reg4_v <= v; // clamp(v, 16, 240);
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
