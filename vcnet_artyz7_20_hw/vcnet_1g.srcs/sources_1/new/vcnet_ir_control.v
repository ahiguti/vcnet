`timescale 1ns / 1ps

module vcnet_ir_control(
input CLK,
input RESETN,
input I_TVALID,
output I_TREADY,
input [15:0] I_TDATA,
input [1:0] I_TSTRB,
input I_TLAST,
output IR_OUT,
output [1:0] DBG_STATE
);

reg [1:0] state;
reg ir_out;
reg [31:0] delay;
reg cur_last;

assign IR_OUT = ir_out;
assign DBG_STATE = state;
assign I_TREADY = (state == 0);

always @(posedge CLK) begin
    if (!RESETN) begin
        state <= 0;
        ir_out <= 1;
        delay <= 0;
        cur_last <= 0;
    end else begin
        if (state == 0) begin
            if (I_TVALID && I_TREADY) begin
                if (I_TSTRB != 0) begin
                    delay <= { 16'h0, I_TDATA[15:0] } * 100;
                    cur_last <= I_TLAST;
                    ir_out <= ~ir_out;
                    state <= 1;
                end else if (I_TLAST) begin
                    ir_out <= 1;
                    delay <= 0;
                    cur_last <= 0;
                end
            end
        end else if (state == 1) begin
            if (delay < 2) begin
                if (cur_last) begin
                    ir_out <= 1;
                    delay <= 0;
                    cur_last <= 0;
                end
                delay <= 0;
                state <= 0;
            end else begin
                delay <= delay - 1;
            end
        end
    end
end

endmodule
