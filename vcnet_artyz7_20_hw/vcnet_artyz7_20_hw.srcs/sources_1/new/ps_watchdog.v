
module ps_watchdog(
    input CLK,
    input RESETN,
    input [3:0] HEARTBEAT_VALUE,
    input [15:0] WATCHDOG_SEC,
    output PS_REBOOT_EN,
    output [41:0] DEBUG_CNT
);

reg ps_reboot_en;
reg [42:0] cnt_usec;
reg [15:0] watchdog_sec;
reg [3:0] heartbeat_value;

assign PS_REBOOT_EN = ps_reboot_en;
assign DEBUG_CNT = cnt_usec;

always @(posedge CLK) begin
    if (!RESETN) begin
        ps_reboot_en <= 0;
        cnt_usec <= 0;
        watchdog_sec <= 0;
        heartbeat_value <= 0;
    end else begin
        watchdog_sec <= WATCHDOG_SEC;
        if (heartbeat_value != HEARTBEAT_VALUE || watchdog_sec == 0) begin
            cnt_usec <= 0;
            heartbeat_value <= HEARTBEAT_VALUE;
        end else begin
            cnt_usec <= cnt_usec + 1;
        end
        if (cnt_usec[42:27] >= watchdog_sec[15:0] && watchdog_sec != 0) begin
            ps_reboot_en <= 1;
        end
    end
end

endmodule
