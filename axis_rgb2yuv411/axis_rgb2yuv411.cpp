
#include "ap_axi_sdata.h"
#include "hls_stream.h"

typedef ap_axiu<24, 1, 0, 0> pkt_rgb_type;
typedef ap_uint<1> u1;
typedef ap_uint<2> u2;
typedef ap_uint<3> u3;
typedef ap_uint<8> u8;
typedef ap_uint<12> u12;
typedef ap_uint<16> u16;
typedef ap_uint<18> u18;
typedef ap_uint<19> u19;
typedef ap_uint<24> u24;
typedef ap_int<18> i18;
typedef ap_int<20> i20;

#if 0

static void
rgb_to_yuv(u24 r_b_g, u24& y_u_v)
{
	u8 r = r_b_g.range(23, 16);
	u8 b = r_b_g.range(15, 8);
	u8 g = r_b_g.range(7, 0);
	u16 y = (r<<6) + (r<<1) + (g<<7) + (g<<0) + (b<<4) + (b<<3) + (b<<0) +                    4096 + 128;
	u16 u = (r<<5) + (r<<2) + (r<<1) + (g<<6) + (g<<3) + (g<<1) + (b<<6) + (b<<5) + (b<<4) + 32768 + 128;
	u16 v = (r<<6) + (r<<5) + (r<<4) + (g<<6) + (g<<5) - (g<<1) + (b<<4) + (b<<1) +          32768 + 128;
	y_u_v.range(23, 16) = y >> 8;
	y_u_v.range(15, 8) = u >> 8;
	y_u_v.range(7, 0) = v >> 8;
}

void axis_rgb2yuv411(u2 fmt, hls::stream<pkt_rgb_type>& s_in, hls::stream<pkt_rgb_type>& s_out)
{
#pragma HLS INTERFACE mode=ap_ctrl_none port=return
#pragma HLS INTERFACE mode=axis port=s_in register
#pragma HLS INTERFACE mode=axis port=s_out register
	u1 is_first_pixel = 1;
	u1 read_line_done = 0;
	u2 xoffset = 0;
	u24 yuv0 = 0;
	u24 yuv1 = 0;
	u24 yuv2 = 0;
	u24 yuv3 = 0;
	u24 yuv4 = 0;
	u24 yuv5 = 0;
	u24 yuv6 = 0;
	u3 ctl0 = 0;
	u3 ctl1 = 0;
	u3 ctl2 = 0;
	u3 ctl3 = 0;
	u3 ctl4 = 0;
	u3 ctl5 = 0;
	u3 ctl6 = 0;
	process_line: while (true) {
#pragma HLS PIPELINE II=1 style=frp
		yuv6 = yuv5; yuv5 = yuv4; yuv4 = yuv3; yuv3 = yuv2; yuv2 = yuv1; yuv1 = yuv0;
		ctl6 = ctl5; ctl5 = ctl4; ctl4 = ctl3; ctl3 = ctl2; ctl2 = ctl1; ctl1 = ctl0;
		if (!read_line_done) {
			pkt_rgb_type rpkt { };
			s_in.read(rpkt);
			u24 r_b_g = rpkt.data;
			rgb_to_yuv(r_b_g, yuv0);
			ctl0[0] = 1;
			ctl0[1] = rpkt.last;
			ctl0[2] = rpkt.user;
			if (is_first_pixel) {
				yuv3 = yuv2 = yuv1 = yuv0;
				is_first_pixel = 0;
			}
			if (rpkt.last) {
				read_line_done = 1;
			}
		} else {
			ctl0 = 0;
		}
		u12 us = (yuv6.range(15,8)<<0) + (yuv5.range(15,8)<<1) + (yuv4.range(15,8)<<0) + (yuv4.range(15,8)<<1) +
				 (yuv0.range(15,8)<<0) + (yuv1.range(15,8)<<1) + (yuv2.range(15,8)<<0) + (yuv2.range(15,8)<<1) +
				 (yuv3.range(15,8)<<2) + 8;
		u12 vs = (yuv6.range(7,0)<<0) + (yuv5.range(7,0)<<1) + (yuv4.range(7,0)<<0) + (yuv4.range(7,0)<<1) +
				 (yuv0.range(7,0)<<0) + (yuv1.range(7,0)<<1) + (yuv2.range(7,0)<<0) + (yuv2.range(7,0)<<1) +
				 (yuv3.range(7,0)<<2) + 8;
		if (ctl3[0]) {
			if (xoffset == 1 || xoffset == 3) {
				u24 oval = 0;
				if (xoffset == 3) {
					oval.range(23, 16) = yuv2.range(23, 16); // y1
					oval.range(16, 8) = yuv3.range(23, 16);  // y0
					oval.range(7, 0) = us >> 4;              // u
				} else if (xoffset == 1) {
					oval.range(23, 16) = yuv2.range(23, 16); // y3
					oval.range(16, 8) = yuv3.range(23, 16);  // y2
					oval.range(7, 0) = vs >> 4;              // v
				}
				pkt_rgb_type wpkt { };
				wpkt.data = oval;
				wpkt.last = ctl3[1];
				wpkt.user = ctl3[2];
				wpkt.strb = 1;
				wpkt.keep = 1;
				s_out.write(wpkt);
			}
		} else if (read_line_done && !is_first_pixel) {
			break;
		}
		++xoffset;
	}
}

#endif

#if 1

static u8
clamp_u8(i18 v)
{
	return v < 0 ? u8(0) : v > 255 ? u8(255) : u8(v);
}

static u16
clamp_u16(i20 v)
{
	return v < 0 ? u16(0) : v > 65536 ? u16(65535) : u16(v);
}

static u18
clamp_u18(i20 v)
{
	return v < 0 ? u18(0) : v > 262144 ? u18(262143) : u18(v);
}

static void
rgb_to_yuv411(u24 rgb0, u24 rgb1, u24 rgb2, u24 rgb3, u24& y1y0u, u24& y3y2v)
{
	i20 g0 = rgb0.range(7, 0);
	i20 b0 = rgb0.range(15, 8);
	i20 r0 = rgb0.range(23, 16);
	i20 g1 = rgb1.range(7, 0);
	i20 b1 = rgb1.range(15, 8);
	i20 r1 = rgb1.range(23, 16);
	i20 g2 = rgb2.range(7, 0);
	i20 b2 = rgb2.range(15, 8);
	i20 r2 = rgb2.range(23, 16);
	i20 g3 = rgb3.range(7, 0);
	i20 b3 = rgb3.range(15, 8);
	i20 r3 = rgb3.range(23, 16);
	i20 y0 = r0 * 55 + g0 * 184 + b0 * 19;
	i20 y1 = r1 * 55 + g1 * 184 + b1 * 19;
	i20 y2 = r2 * 55 + g2 * 184 + b2 * 19;
	i20 y3 = r3 * 55 + g3 * 184 + b3 * 19;
	i20 r = r0 + r1 + r2 + r3;
	i20 g = g0 + g1 + g2 + g3;
	i20 b = b0 + b1 + b2 + b3;
	i20 u = r * (-29) + g * (-99) + b * 128 + 32768 * 4;
	i20 v = r * 129 + g * (-116) + b * (-12) + 32768 * 4;
	u16 y0u = clamp_u16(y0);
	u16 y1u = clamp_u16(y1);
	u16 y2u = clamp_u16(y2);
	u16 y3u = clamp_u16(y3);
	u18 uu = clamp_u18(u);
	u18 vu = clamp_u18(v);
	y0u >>= 8;
	y1u >>= 8;
	y2u >>= 8;
	y3u >>= 8;
	uu >>= 10;
	vu >>= 10;
	y1y0u.range(7, 0) = uu.range(7, 0);
	y1y0u.range(15, 8) = y0u.range(7, 0);
	y1y0u.range(23, 16) = y1u.range(7, 0);
	y3y2v.range(7, 0) = vu.range(7, 0);
	y3y2v.range(15, 8) = y2u.range(7, 0);
	y3y2v.range(23, 16) = y3u.range(7, 0);
}

void axis_rgb2yuv411(u2 fmt, hls::stream<pkt_rgb_type>& s_in, hls::stream<pkt_rgb_type>& s_out)
{
#pragma HLS INTERFACE mode=ap_ctrl_none port=return
#pragma HLS INTERFACE mode=axis port=s_in
#pragma HLS INTERFACE mode=axis port=s_out
	if (fmt == 2) {
		process_pkt_yuv411: while (true) {
#pragma HLS PIPELINE II=4
			pkt_rgb_type pkt0 { };
			pkt_rgb_type pkt1 { };
			pkt_rgb_type pkt2 { };
			pkt_rgb_type pkt3 { };
			s_in.read(pkt0);
			if (!pkt0.last) {
				s_in.read(pkt1);
				if (!pkt1.last) {
					s_in.read(pkt2);
					if (!pkt2.last) {
						s_in.read(pkt3);
					}
				}
			}
			pkt_rgb_type opkt0 { };
			pkt_rgb_type opkt1 { };
			rgb_to_yuv411(pkt0.data, pkt1.data, pkt2.data, pkt3.data, opkt0.data, opkt1.data);
			opkt0.user = pkt0.user;
			opkt0.last = pkt0.last;
			opkt0.keep = 7;
			opkt0.strb = 7;
			opkt1.user = 0;
			opkt1.last = pkt1.last;
			opkt1.keep = 7;
			opkt1.strb = 7;
			s_out.write(opkt0);
			if (!opkt0.last) {
				opkt1.last = pkt1.last | pkt2.last | pkt3.last;
				s_out.write(opkt1);
			}
			if (pkt0.last || pkt1.last || pkt2.last || pkt3.last) {
				break;
			}
		}
	} else {
		process_pkt_rgb: while (true) {
#pragma HLS PIPELINE II=1
			pkt_rgb_type pkt0 { };
			s_in.read(pkt0);
			u8 v0 = pkt0.data.range(7, 0);
			u8 v1 = pkt0.data.range(15, 8);
			pkt0.data.range(7, 0) = v1;
			pkt0.data.range(15, 8) = v0;
			s_out.write(pkt0);
			if (pkt0.last) {
				break;
			}
		}
	}
}

#endif
