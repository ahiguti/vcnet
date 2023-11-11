module yuv444_to_yuv411(
input CLK,
input I_DE, I_VSYNC, I_HSYNC, I_USER,
input [7:0] I_Y, I_U, I_V,
output O_DE, O_VSYNC, O_HSYNC, O_USER, O_LAST,
output [7:0] O_Y0, O_Y1, O_UV,
output [1:0] O_OFFSET,
output O_VALID
);

reg [3:0] ctl[0:6];
reg [23:0] yuv[0:6];
reg [11:0] u_sum0_4, u_sum1_4, u_sum2_4, v_sum0_4, v_sum1_4, v_sum2_4;
reg [11:0] u_sum_5, v_sum_5;
reg [1:0] xoffset_5, xoffset_6;
reg [7:0] uv_6;
reg [7:0] y_7;
reg valid_5, valid_6;
reg last_6;

integer i;

always @(posedge CLK) begin
    ctl[0] <= { I_USER, I_HSYNC, I_VSYNC, I_DE };
    yuv[0] <= I_DE ? { I_Y, I_U, I_V } : 24'h108080;
    for (i = 0; i < 6; i = i + 1) begin
        ctl[i + 1] <= ctl[i];
        yuv[i + 1] <= yuv[i];
    end
    u_sum0_4 <= (yuv[6][15:8]<<0) + (yuv[5][15:8]<<1) + (yuv[4][15:8]<<0);
    u_sum1_4 <= (yuv[4][15:8]<<1) + (yuv[3][15:8]<<2) + (yuv[2][15:8]<<0);
    u_sum2_4 <= (yuv[2][15:8]<<1) + (yuv[1][15:8]<<1) + (yuv[0][15:8]<<0);
    v_sum0_4 <= (yuv[6][7:0]<<0) + (yuv[5][7:0]<<1) + (yuv[4][7:0]<<0);
    v_sum1_4 <= (yuv[4][7:0]<<1) + (yuv[3][7:0]<<2) + (yuv[2][7:0]<<0);
    v_sum2_4 <= (yuv[2][7:0]<<1) + (yuv[1][7:0]<<1) + (yuv[0][7:0]<<0);
    u_sum_5 <= u_sum0_4 + u_sum1_4 + u_sum2_4;
    v_sum_5 <= v_sum0_4 + v_sum1_4 + v_sum2_4;
    if (ctl[4][0]) begin
        if (!ctl[5][0]) begin
            xoffset_5 <= 0;
        end else begin
            xoffset_5 <= xoffset_5 + 1;
        end
    end
    xoffset_6 <= xoffset_5;
    if (xoffset_5 == 0) begin
        uv_6 <= u_sum_5 >> 4;
    end else if (xoffset_5 == 2) begin
        uv_6 <= v_sum_5 >> 4;
    end
    valid_6 <= ctl[5][0] && xoffset_5[0];
    last_6 <= valid_5 && (!ctl[4][0] || !ctl[3][0]);
    y_7 <= yuv[6][23:16];
end

assign O_DE = ctl[6][0];
assign O_VSYNC = ctl[6][1];
assign O_HSYNC = ctl[6][2];
assign O_USER = ctl[6][3];
assign O_Y1 = yuv[6][23:16];
assign O_Y0 = y_7;
assign O_UV = uv_6;
assign O_OFFSET = xoffset_6;
assign O_VALID = valid_6; 
assign O_LAST = last_6;

endmodule
