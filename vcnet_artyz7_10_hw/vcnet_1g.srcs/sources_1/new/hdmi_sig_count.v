`timescale 1ns / 1ps

module hdmi_sig_count(
input CLK,
input RESETN,
input DATA_EN,
input [23:0] DATA,
input VSYNC,
input HSYNC,
output [15:0] VCOUNT,
output [15:0] HCOUNT,
output [31:0] PIXCOUNT,
output [15:0] VCOUNT_SAVE,
output [15:0] HCOUNT_SAVE,
output [31:0] PIXCOUNT_SAVE,
output [31:0] COUNT_FRAME_EQ,
output [31:0] COUNT_FRAME_NE,
output INTERLACED,
output ODD_FRAME,
output [127:0] DEBUG_CHECKSUM0,
output [127:0] DEBUG_CHECKSUM1
);

reg vsync0;
reg vsync1;
reg hsync0;
reg hsync1;
reg data_en0;
reg data_en1;
reg [23:0] data0;
reg [23:0] data1;
reg [15:0] vcount;
reg [15:0] hcount;
reg [31:0] pixcount;
reg [15:0] vcount_save;
reg [15:0] hcount_save;
reg [31:0] pixcount_save;
reg odd_frame;
reg odd_frame1;

assign VCOUNT = vcount;
assign HCOUNT = hcount;
assign PIXCOUNT = pixcount;
assign VCOUNT_SAVE = vcount_save;
assign HCOUNT_SAVE = hcount_save;
assign PIXCOUNT_SAVE = pixcount_save;
assign INTERLACED = (odd_frame || odd_frame1);
assign ODD_FRAME = odd_frame;

always @(posedge CLK) begin
    if (!RESETN) begin
        vsync0 <= 0;
        hsync0 <= 0;
        data_en0 <= 0;
        data0 <= 0;
        vcount <= 0;
        hcount <= 0;
        pixcount <= 0;
        vcount_save <= 0;
        hcount_save <= 0;
        pixcount_save <= 0;
        odd_frame <= 0;
        odd_frame1 <= 0;
    end else begin
        vsync0 <= VSYNC;
        vsync1 <= vsync0;
        hsync0 <= HSYNC;
        hsync1 <= hsync0;
        data_en0 <= DATA_EN;
        data_en1 <= data_en0;
        data0 <= DATA;
        data1 <= data0;
        if (data_en1) begin
            pixcount <= pixcount + 1;
            hcount <= hcount + 1;
        end
        if (vsync1 == 0 && vsync0 == 1) begin
            if (pixcount != 0) begin
                pixcount_save <= pixcount;
            end
            pixcount <= 0;
            if (vcount != 0) begin
                vcount_save <= vcount;
            end
            vcount <= 0;
            odd_frame <= (hsync0 == 0);
            odd_frame1 <= odd_frame;
        end
        if (hsync1 == 0 && hsync0 == 1) begin
            if (hcount != 0) begin
                vcount <= vcount + 1;
                hcount_save <= hcount;
            end
            hcount <= 0;
        end
    end
end

localparam pipeline_len = 4;
reg [127:0] hdata;
reg hdata_en;
reg [127:0] checksum, checksum_prev;
reg [31:0] cnt_eq, cnt_ne;

assign COUNT_FRAME_EQ = cnt_eq;
assign COUNT_FRAME_NE = cnt_ne;
assign DEBUG_CHECKSUM0 = checksum;
assign DEBUG_CHECKSUM1 = checksum_prev;

wire [127:0] hdin = hdata ^ checksum;
wire [127:0] hval;

aesenc_noreg aesenc_inst(.CLK(CLK), .IN_DATA(hdin), .OUT_DATA(hval));

always @(posedge CLK) begin
    if (!RESETN) begin
        hdata <= 0;
        hdata_en <= 0;
        checksum <= 0;
        checksum_prev <= 0;
        cnt_eq <= 0;
        cnt_ne <= 0;
    end else begin
        hdata[23:0] <= data1;
        hdata_en <= data_en1;
        if (hdata_en) begin
            checksum <= hval;
        end
        if (vsync1 == 0 && vsync0 == 1) begin
            checksum <= 0;
            checksum_prev <= checksum;
            if (checksum == checksum_prev) begin
                cnt_eq <= cnt_eq + 1;
            end else begin
                cnt_ne <= cnt_ne + 1;
            end
        end
    end
end

endmodule
