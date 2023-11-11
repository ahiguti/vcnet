`timescale 1ns / 1ps

module sim_add_dummy();

reg clk;
reg resetn;

initial begin
    clk = 0;
    forever clk = #1 !clk;
end

initial begin
    resetn = 0;
    #20 resetn = 1;
end

reg [7:0] count;

always @(posedge clk) begin
    if (!resetn) begin
        count <= 0;
    end else begin
        count <= count + 1;
    end
end

wire de = (count > 10) && (count < 80);
wire valid = de && (count[0]);
wire hsync = (count == 100);
wire vsync = (count == 110);
wire [23:0] data = count;

yuv411_add_dummy_pixels inst(.CLK(clk), .RESETN(resetn), .I_DE(de), .I_VALID(valid), .I_VSYNC(vsync), .I_HSYNC(hsync), .I_Y0(data[7:0]), .I_Y1(data[15:8]), .I_UV(data[23:16]));

endmodule
