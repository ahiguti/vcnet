`timescale 1ns / 1ps

module sim_rgb2yuv422();

reg clk;
reg resetn;
reg [23:0] rgb;
reg [31:0] cnt;

initial begin
clk = 0;
resetn = 0;
#100;
resetn = 1;
end

always begin
#10 clk = ~clk;
end

always @(posedge clk) begin
    if (!resetn) begin
        rgb <= 0;
        cnt <= 0;
    end else begin
        rgb <= rgb + 1;
        cnt <= cnt + 1;
        if (cnt >= 2000) begin
            cnt <= 0;
        end
    end
end

wire rgb_valid = (cnt < 1920);

wire ap_ctrl_ready;
wire s_in_tready;
wire s_out_tvalid;
wire [15:0] s_out_tdata;

design_1_wrapper inst(
    .ap_clk(clk), .ap_rst_n(resetn), .ap_ctrl_start(1), .ap_ctrl_ready(ap_ctrl_ready),
    .s_in_tvalid(rgb_valid), .s_in_tready(s_in_tready), .s_in_tdata(rgb),
    .s_out_tvalid(s_out_tvalid), .s_in_tuser(0), .s_out_tready(1), .s_out_tdata(s_out_tdata));

endmodule
