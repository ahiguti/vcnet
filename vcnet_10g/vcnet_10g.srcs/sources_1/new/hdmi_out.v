
// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module hdmi_out(
input wire VID_CLK,
input wire [23:0] VID_DATA,
input wire VID_VBLANK,
input wire VID_HBLANK,
input wire VID_VSYNC,
input wire VID_HSYNC,
output wire [23:0] HDMI_D,
output wire HDMI_DE,
output wire HDMI_VSYNC,
output wire HDMI_HSYNC
);

assign HDMI_DE = (!VID_VBLANK) && (!VID_HBLANK);
assign HDMI_D = HDMI_DE ? VID_DATA[23:0] : 24'h0;
assign HDMI_VSYNC = !VID_VSYNC;
assign HDMI_HSYNC = !VID_HSYNC;

endmodule
