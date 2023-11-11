
// Copyright (C) 2017 Akira Higuchi. All rights reserved.
// See COPYRIGHT.txt for details.

module video_out_444(
    input [23:0] VID_IN,
    input VID_IN_VBLANK,
    input VID_IN_HBLANK,
    output [11:0] VID_OUT
    );
assign VID_OUT = (VID_IN_VBLANK || VID_IN_HBLANK) ? 24'h0 : { VID_IN[23:20], VID_IN[15:12], VID_IN[7:4] };
endmodule
