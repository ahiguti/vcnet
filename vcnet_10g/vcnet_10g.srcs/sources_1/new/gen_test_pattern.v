
// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module gen_test_pattern(
    input CLK,
    input RESETN,
    input [7:0] SW,
    input TP_TREADY,
    output TP_TVALID,
    output [23:0] TP_TDATA,
    output TP_TLAST,
    output TP_TUSER
    );
localparam xsize = 1920;
localparam ysize = 1080;
reg [11:0] x;
reg [11:0] y;
reg [7:0] swval;
reg [7:0] swsaved;

wire [7:0] data_r = { x[5:0], 2'b0 } | swval;
wire [7:0] data_g = { y[5:0], 2'b0 } | swval;
wire [7:0] data_b = { x[9:6], y[9:6] } | swval;
//wire [7:0] data_r = { SW[7:4], 4'h0 } | swval;
//wire [7:0] data_g = { SW[3:2], 6'h0 } | swval;
//wire [7:0] data_b = { SW[1:0], 6'h0 } | swval;

assign TP_TVALID = 1;
assign TP_TDATA = { data_r, data_b, data_g };
assign TP_TLAST = (x == xsize - 1);
assign TP_TUSER = (x == 0 && y == 0);

always @(posedge CLK) begin
    if (!RESETN) begin
        x <= xsize - 1;
        y <= ysize - 1;
        swsaved <= 0;
        swval <= 0;
    end else begin
        if (TP_TREADY) begin
            if (x < 256) begin
                swval <= ((SW >> (x / 32)) & 1'b1) ? 8'h7f : 8'h00;
            end else if (x < 512) begin
                swval <= ((swsaved >> ((x & 255)/ 32)) & 1'b1) ? 8'h7f : 8'h00;
            end else begin
                swval <= 0;
            end
            x <= x + 1;
            if (x == xsize - 1) begin
                x <= 0;
                y <= y + 1;
                if (y == ysize - 1) begin
                    y <= 0;
                    swsaved <= SW;
                end
            end
        end
    end
end

endmodule
