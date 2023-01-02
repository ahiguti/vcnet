`timescale 1ns / 1ps

module sim_watchdog();

reg CLK;
reg RESETN;
reg [3:0] HEARTBEAT_VALUE;
reg [15:0] WATCHDOG_SEC;
wire PS_REBOOT_EN;
wire [41:0] DEBUG_CNT;

initial begin
    RESETN = 0;
    CLK = 0;
    HEARTBEAT_VALUE <= 10;
    WATCHDOG_SEC <= 3;
#100
    RESETN = 1;
end

always #10 CLK <= ~CLK;

ps_watchdog wd(.CLK(CLK), .RESETN(RESETN), .HEARTBEAT_VALUE(HEARTBEAT_VALUE), .WATCHDOG_SEC(WATCHDOG_SEC), .PS_REBOOT_EN(PS_REBOOT_EN), .DEBUG_CNT(DEBUG_CNT));

endmodule
