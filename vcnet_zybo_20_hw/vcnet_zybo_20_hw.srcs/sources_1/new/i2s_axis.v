
module i2s_axis(
    input CLK,
    input RESETN,
    input I2S_BCLK,
    input I2S_LR,
    input I2S_DAT,
    input S_TREADY,
    output S_TVALID,
    output [31:0] S_TDATA,
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

assign bclk = sync_bclk[2];
assign lr = sync_lr[2];
assign dat = sync_dat[2];

assign S_TVALID = s_tvalid;
assign S_TDATA = s_tdata;   /* s_tdata <= { data[31:8], lr[0:0], 1'b0, nbits[5:0] }; */
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
    end else begin
        s_tvalid <= 0;
        bclk_1 <= bclk;
        if (bclk_1 == 0 && bclk == 1) begin
            lr_1 <= lr;
            lr_2 <= lr_1;
            if (lr_2 != lr_1) begin
                s_tvalid <= 1; // emit data
                if (lr_1) begin
                    s_tdata <= { right[31:8], 2'b10, rbits[5:0] };
                    right <= { dat, 31'b0 };
                    rbits <= 1;
                end else begin
                    s_tdata <= { left[31:8], 2'b00, lbits[5:0] };
                    left <= { dat, 31'b0 };
                    lbits <= 1;
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
    end
end

endmodule
