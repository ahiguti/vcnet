`timescale 1ns / 1ps

// RGB 24bitを64bit幅のaxi streamに変換する。TREADYは無し。
// DISP_WIDTHは8の倍数でないといけない。

module rgb_to_axis(
input CLK,
input RESETN,
input [15:0] DISP_WIDTH,
input [15:0] DISP_HEIGHT,
input INTERLACED,
input ODD_FRAME,
input HSYNC,
input VSYNC,
input DATA_EN,
input [23:0] DATA,
input DISABLE_VIDEO,
output O_TVALID,
output [63:0] O_TDATA,
output [3:0] O_TUSER,
output O_TLAST,
output DBG_STATE,
output [63:0] DBG_WDATA,
output [7:0] DBG_WDATA_BYTES,
output [15:0] DBG_WORD_COUNT,
output [15:0] DBG_LINE_COUNT
);

(* ASYNC_REG = "TRUE" *) reg [2:0] metastability_guard_disable_video;

reg [15:0] i_disp_width;
reg [15:0] i_disp_height;
reg i_interlaced;
reg i_odd_frame;
reg i_hsync;
reg i_vsync;
reg i_vsync1;
reg i_data_en;
reg i_data_en1;
reg [23:0] i_data;

reg o_tvalid;
reg [63:0] o_tdata;
reg [3:0] o_tuser;
reg o_tlast;

reg state;
reg enable_out;
reg [15:0] cur_disp_width;
reg [15:0] cur_disp_height;
reg [7:0] wdata [0:7];
reg [7:0] wdata_bytes;
reg [15:0] word_count;
reg [15:0] line_count;
reg [15:0] frame_count;
reg [23:0] debug_count; // デバッグ用

wire disable_video = metastability_guard_disable_video[2];

assign O_TVALID = o_tvalid;
assign O_TDATA = o_tdata;
assign O_TUSER = o_tuser;
assign O_TLAST = o_tlast;
assign DBG_STATE = state;
assign DBG_WDATA = { wdata[7], wdata[6], wdata[5], wdata[4], wdata[3], wdata[2], wdata[1], wdata[0] };
assign DBG_WDATA_BYTES = wdata_bytes;
assign DBG_WORD_COUNT = word_count;
assign DBG_LINE_COUNT = line_count;

integer i;

always @(posedge CLK) begin
    metastability_guard_disable_video[2:0] <= { metastability_guard_disable_video[1:0], DISABLE_VIDEO };
end

always @(posedge CLK) begin
    if (!RESETN) begin
        i_disp_width <= 0;
        i_disp_height <= 0;
        i_interlaced <= 0;
        i_odd_frame <= 0;
        i_hsync <= 0;
        i_vsync <= 0;
        i_vsync1 <= 0;
        i_data_en <= 0;
        i_data_en1 <= 0;
        i_data <= 0;
        o_tvalid <= 0;
        o_tdata <= 0;
        o_tuser <= 0;
        o_tlast <= 0;
        state <= 0;
        enable_out <= 0;
        cur_disp_width <= 0;
        cur_disp_height <= 0;
        for (i = 0; i < 8; i = i + 1) begin
            wdata[i] <= 0;
        end
        wdata_bytes <= 0;
        word_count <= 0;
        line_count <= 0;
        frame_count <= 0;
        debug_count <= 0;
    end else begin
        i_disp_width <= DISP_WIDTH;
        i_disp_height <= DISP_HEIGHT;
        i_interlaced <= INTERLACED;
        i_odd_frame <= ODD_FRAME;
        i_hsync <= HSYNC;
        i_vsync <= VSYNC;
        i_vsync1 <= i_vsync;
        i_data_en <= DATA_EN;
        i_data_en1 <= i_data_en;
        i_data <= DATA;
        // i_data <= debug_count; // デバッグ用
        o_tvalid <= 0;
        o_tdata <= 0;
        o_tuser <= 0;
        o_tlast <= 0;
        if (state == 0 && i_vsync == 1 && i_vsync1 == 0) begin
            line_count <= 0;
            frame_count <= frame_count + 1;
            enable_out <= !disable_video;
        end
        if (state == 0 && i_disp_width != 0 && i_data_en == 1 && i_data_en1 == 0) begin
            // 一行の出力を開始する。一行を一つのUDPパケットにする。最初の8byteにヘッダ。
            state <= 1;
            cur_disp_width <= i_disp_width;
            cur_disp_height <= i_disp_height;
            wdata[0] <= i_data[7:0];
            wdata[1] <= i_data[15:8];
            wdata[2] <= i_data[23:16];
            debug_count <= 1;
            wdata_bytes <= 3;
            word_count <= 0;
            // ヘッダ8byteを出力。下位から16bitずつ、(データの長さ、現在の行、高さ,フレームカウント)。
            // ただし最上位8bitは0にする。ビデオデータであることを示すタグ。
            o_tdata <= { 8'b0, i_odd_frame, i_interlaced, frame_count[5:0], i_disp_height[15:0], line_count[15:0], i_disp_width[15:0] * 16'd3 + 16'd8 };
            o_tvalid <= enable_out;
            o_tuser <= 8;
            o_tlast <= 0;
        end
        if (state == 1) begin
            wdata[(wdata_bytes + 0) & 7] <= i_data[7:0];
            wdata[(wdata_bytes + 1) & 7] <= i_data[15:8];
            wdata[(wdata_bytes + 2) & 7] <= i_data[23:16];
            debug_count <= debug_count + 1;
            wdata_bytes <= (wdata_bytes + 3) & 7;
            if (wdata_bytes >= 5) begin
                o_tdata <= { wdata[7], wdata[6], wdata[5], wdata[4], wdata[3], wdata[2], wdata[1], wdata[0] };
                if (wdata_bytes == 5) begin
                    o_tdata[63:40] <= i_data[23:0];
                end else if (wdata_bytes == 6) begin
                    o_tdata[63:48] <= i_data[15:0];
                end else if (wdata_bytes == 7) begin
                    o_tdata[63:56] <= i_data[7:0];
                end
                o_tvalid <= enable_out;
                o_tuser <= 8;
                word_count <= word_count + 1;
                if ((word_count + 1) * 8 >= cur_disp_width * 3) begin
                    o_tlast <= 1;
                    wdata_bytes <= 0;
                    word_count <= 0;
                    line_count <= line_count + 1;
                    state <= 0;
                end
            end
        end
    end
end

endmodule
