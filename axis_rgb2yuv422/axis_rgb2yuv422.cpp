
#include "ap_axi_sdata.h"
#include "hls_stream.h"

typedef ap_axiu<24, 1, 0, 0> pkt_rgb_type;
typedef ap_axiu<16, 1, 0, 0> pkt_yuv422_type;
typedef ap_uint<1> u1;
typedef ap_uint<8> u8;
typedef ap_uint<16> u16;
typedef ap_uint<24> u24;
typedef ap_int<17> i17;

static u8
clamp_u8(i17 v)
{
	return v < 0 ? u8(0) : v > 255 ? u8(255) : u8(v);
}

static void
rgb_to_yuv422(u24 rgb0, u24 rgb1, u16& y0u, u16& y1v)
{
	i17 g0 = rgb0.range(7, 0);
	i17 b0 = rgb0.range(15, 8);
	i17 r0 = rgb0.range(23, 16);
	i17 g1 = rgb1.range(7, 0);
	i17 b1 = rgb1.range(15, 8);
	i17 r1 = rgb1.range(23, 16);
	i17 y0 = r0 * 66 + g0 * 128 + b0 * 25 + 128;
	i17 y1 = r1 * 66 + g1 * 128 + b1 * 25 + 128;
	i17 u = (r0 + r1) * (-19) + (g0 + g1) * (-37) + (b0 + b1) * 56 + 128;
	i17 v = (r0 + r1) * 56 + (g0 + g1) * (-47) + (b0 + b1) * (-9) + 128;
	y0 >>= 8;
	y1 >>= 8;
	u >>= 8;
	v >>= 8;
	y0 += 16;
	y1 += 16;
	u += 128;
	v += 128;
	y0u = clamp_u8(y0).concat(clamp_u8(u));
	y1v = clamp_u8(y1).concat(clamp_u8(v));
}

void axis_rgb2yuv422(hls::stream<pkt_rgb_type>& s_in, hls::stream<pkt_yuv422_type>& s_out)
{
#pragma HLS INTERFACE mode=ap_ctrl_none port=return
#pragma HLS INTERFACE mode=axis port=s_in
#pragma HLS INTERFACE mode=axis port=s_out
#if 0
	while (true) {
		while (true) {
			pkt_rgb_type pkt { };
			s_in.read(pkt);
			pkt_yuv422_type opkt { };
			opkt.data = pkt.data;
			opkt.last = pkt.last;
			opkt.user = pkt.user;
			s_out.write(opkt);
			if (pkt.last) {
				break;
			}
		}
	}
#endif
#if 1
	process_pkt: while (true) {
#pragma HLS PIPELINE II=2
		pkt_rgb_type pkt0 { };
		pkt_rgb_type pkt1 { };
		pkt_yuv422_type opkt0 { };
		pkt_yuv422_type opkt1 { };
		s_in.read(pkt0);
		if (!pkt0.last) {
			s_in.read(pkt1);
		}
		rgb_to_yuv422(pkt0.data, pkt1.data, opkt0.data, opkt1.data);
		opkt0.last = pkt0.last;
		opkt0.user = pkt0.user;
		opkt1.last = pkt1.last;
		opkt1.user = pkt1.user;
		s_out.write(opkt0);
		if (!pkt0.last) {
			s_out.write(opkt1);
		}
	}
#endif
#if 0
	while (true) {
		pkt_rgb_type pkt { };
		pkt_yuv422_type opkt { };
		bool odd = false;
		u24 rgb0 { };
		u24 rgb1 { };
		u1 user0 { };
		u1 user1 { };
		u16 y0u { };
		u16 y1v { };
		s_in.read(pkt);
		rgb0 = pkt.data;
		user0 = pkt.user;
		odd = true;
		process_pkt: while (true) {
#pragma HLS PIPELINE II=1
			s_in.read(pkt);
			if (pkt.last) {
				break;
			}
			if (!odd) {
				rgb0 = pkt.data;
				user0 = pkt.user;
				opkt.data = y1v;
				opkt.last = false;
				opkt.user = user1;
			} else {
				rgb1 = pkt.data;
				user1 = pkt.user;
				rgb_to_yuv422(rgb0, rgb1, y0u, y1v);
				opkt.data = y0u;
				opkt.last = false;
				opkt.user = user0;
			}
			s_out.write(opkt);
			odd = !odd;
		}
		if (!odd) {
			opkt.data = y1v;
			opkt.user = user1;
		} else {
			opkt.data = y0u;
			opkt.user = user0;
		}
		opkt.last = true;
		s_out.write(opkt);
	}
#endif
}

