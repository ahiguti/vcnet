
module regs_i2c_master(
input CLK,
input RESETN,
input RD_ADDR_EN,
input [7:0] RD_ADDR,
output reg RD_DATA_EN,
output reg [31:0] RD_DATA,
input WR_EN,
input [7:0] WR_ADDR,
input [31:0] WR_DATA,
input I2C_READY,
output reg I2C_VALID,
output reg [6:0] I2C_DEVADDR,
output reg [31:0] I2C_WDATA,
output reg [2:0] I2C_WBYTES,
input [31:0] I2C_RDATA,
output reg [2:0] I2C_RBYTES,
input [2:0] I2C_ERROR,
output reg [3:0] LED,
output reg IR_OUT,
output reg [15:0] WATCHDOG,
input [31:0] I2S_DATA,
output reg I2S_PULL_DATA,
output reg [31:0] DEBUG_I2S_INSANE,
output reg DEBUG_I2S_INSANE_TRIG,
output reg [31:0] DEBUG_I2S_PREV
);

always @(posedge CLK) begin
  if (!RESETN) begin
    RD_DATA_EN <= 0;
    RD_DATA <= 0;
    I2C_VALID <= 0;
    I2C_DEVADDR <= 0;
    I2C_WDATA <= 0;
    I2C_WBYTES <= 0;
    I2C_RBYTES <= 0;
    LED <= 0;
    WATCHDOG <= 0;
    I2S_PULL_DATA <= 0;
    DEBUG_I2S_INSANE <= 0;
    DEBUG_I2S_INSANE_TRIG <= 0;
    DEBUG_I2S_PREV  <= 0;
  end else begin
    RD_DATA_EN <= 0;
    I2S_PULL_DATA <= 0;
    DEBUG_I2S_INSANE_TRIG <= 0;
    if (RD_ADDR_EN) begin
        RD_DATA_EN <= 1;
        RD_DATA <= 0;
        case (RD_ADDR)
        0: RD_DATA <= { 24'h0, I2C_READY, 4'b0, I2C_ERROR[2:0] };
        6: RD_DATA <= I2C_RDATA;
        14: begin
            RD_DATA <= I2S_DATA;
            I2S_PULL_DATA <= 1;
            if (I2S_DATA != 0) begin
                if (DEBUG_I2S_PREV[7] == I2S_DATA[7]) begin
                    DEBUG_I2S_INSANE <= DEBUG_I2S_INSANE + 1;
                    DEBUG_I2S_INSANE_TRIG <= 1;
                end
                DEBUG_I2S_PREV <= I2S_DATA;
            end
        end
        endcase
    end
    if (WR_EN) begin
        case (WR_ADDR)
        0:
          begin
            I2C_DEVADDR <= WR_DATA[6:0];
            I2C_VALID <= 1;
          end
        2:
          begin
            I2C_WDATA <= WR_DATA[31:0];
          end
        4: I2C_WBYTES <= WR_DATA[2:0];
        8: I2C_RBYTES <= WR_DATA[2:0];
        10: LED <= WR_DATA[3:0];
        12: WATCHDOG <= WR_DATA[15:0];
        16: IR_OUT <= WR_DATA[0];
        endcase
    end
    if (!I2C_READY) begin
        I2C_VALID <= 0;
    end
  end
end

endmodule

