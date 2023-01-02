
module i2s_axis(
    input CLK,
    input RESETN,
    input I2S_BCLK,
    input I2S_LR,
    input I2S_DAT,
    input S_TREADY,
    output S_TVALID,
    output [31:0] S_TDATA,
    output S_TLAST,
    output [31:0] DBG_LEFT,
    output [31:0] DBG_RIGHT,
    output [7:0] DBG_LBITS,
    output [7:0] DBG_RBITS,
    output DBG_LR
);

(* ASYNC_REG = "TRUE" *) reg [2:0] sync_bclk;
(* ASYNC_REG = "TRUE" *) reg [2:0] sync_lr;
(* ASYNC_REG = "TRUE" *) reg [2:0] sync_dat;

reg [31:0] left;
reg [31:0] right;
reg [5:0] lbits;
reg [5:0] rbits;
reg bclk_1;
reg lr_1;
reg lr_2;
reg s_tvalid;
reg [31:0] s_tdata;
reg s_tlast;
reg [7:0] sample_count;
reg [1:0] header_state;

wire bclk = sync_bclk[2];
wire lr = sync_lr[2];
wire dat = sync_dat[2];

assign S_TVALID = s_tvalid;
assign S_TDATA = s_tdata;   /* s_tdata <= { data[31:8], lr[0:0], 1'b0, nbits[5:0] }; */
assign S_TLAST = s_tlast;
assign DBG_LEFT = left;
assign DBG_RIGHT = right;
assign DBG_LBITS = lbits;
assign DBG_RBITS = rbits;
assign DBG_LR = lr;

always @(posedge CLK) begin
    sync_bclk[2:0] <= { sync_bclk[1:0], I2S_BCLK };
    sync_lr[2:0] <= { sync_lr[1:0], I2S_LR };
    sync_dat[2:0] <= { sync_dat[1:0], I2S_DAT };
end

always @(posedge CLK) begin
    if (!RESETN) begin
        left <= 0;
        right <= 0;
        lbits <= 0;
        rbits <= 0;
        bclk_1 <= 0;
        lr_1 <= 0;
        lr_2 <= 0;
        s_tvalid <= 0;
        s_tdata <= 0;
        s_tlast <= 0;
        sample_count <= 0;
        header_state <= 0;
    end else begin
        s_tvalid <= 0;
        bclk_1 <= bclk;
        if (bclk_1 == 0 && bclk == 1) begin
            lr_1 <= lr;
            lr_2 <= lr_1;
            if (lr_2 != lr_1) begin
                s_tvalid <= 1; // emit data
                s_tlast <= 0;
                if (lr_1) begin
                    s_tdata <= { right[31:8], 2'b10, rbits[5:0] };
                    right <= { dat, 31'b0 };
                    rbits <= 1;
                end else begin
                    s_tdata <= { left[31:8], 2'b00, lbits[5:0] };
                    left <= { dat, 31'b0 };
                    lbits <= 1;
                end
                sample_count <= sample_count + 1;
                if (sample_count == 7'hff) begin
                    s_tlast <= 1;
                    header_state <= 2;
                end
            end else begin
                if (lr_1) begin
                    right[31-rbits] <= dat;
                    rbits <= rbits + 1;
                end else begin
                    left[31-lbits] <= dat;
                    lbits <= lbits + 1;
                end
            end
        end
        if (header_state == 2) begin
            // ヘッダの最初の32bitを出力。最初の16bitにはフレーム長。
            s_tvalid <= 1;
            s_tdata <= {16'd0, 16'd8 + 16'd4 * 16'h100 };
            s_tlast <= 0;
            header_state <= 1;
        end else if (header_state == 1) begin
            // ヘッダの残りの32bitを出力。最上位byteが1であるのがaudioフレームの印。
            s_tvalid <= 1;
            s_tdata <= { 8'h01, 24'h0 };
            s_tlast <= 0;
            header_state <= 0;
        end
    end
end

endmodule
