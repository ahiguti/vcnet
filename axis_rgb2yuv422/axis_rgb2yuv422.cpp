
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

#if 1

struct pixbuf {
	ap_uint<96> pix;
	u8 npixbytes;
	u16 nzerobytes;
	u1 user;
};

void out_yuv422(hls::stream<pkt_rgb_type>& s_out, pixbuf& pb, u1 user, u16 v)
{
#pragma HLS INLINE
	pb.user |= user;
	if (pb.npixbytes == 0) {
		pb.pix.range(15, 0) = v;
		pb.npixbytes = 2;
	} else if (pb.npixbytes == 1) {
		pb.pix.range(23, 8) = v;
		pb.npixbytes = 3;
	} else if (pb.npixbytes == 2) {
		pb.pix.range(31, 16) = v;
		pb.npixbytes = 4;
	}
	if (pb.npixbytes >= 3) {
		pkt_rgb_type opkt { };
		opkt.user = pb.user;
		opkt.data = pb.pix.range(23, 0);
		s_out.write(opkt);
		pb.user = 0;
		pb.pix >>= 24;
		pb.npixbytes -= 3;
	}
}

void out_yuv422_rem(hls::stream<pkt_rgb_type>& s_out, pixbuf& pb)
{
#pragma HLS INLINE
	pkt_rgb_type opkt { };
	opkt.data = pb.pix.range(23, 0);
	opkt.last = 1;
	s_out.write(opkt);
#if 0
	pb.nzerobytes += pb.npixbytes;
	pb.npixbytes = 0;
	if (pb.nzerobytes != 0) {
		if (pb.nzerobytes > 2) {
			pb.nzerobytes -= 3;
		} else {
			pb.nzerobytes = 0;
		}
		pkt_rgb_type opkt { };
		opkt.data = pb.pix.range(23, 0);
		if (pb.nzerobytes == 0) {
			opkt.last = 1;
		}
		s_out.write(opkt);
		pb.pix.range(23, 0) = 0;
	}
#endif
}

void axis_rgb2yuv422(hls::stream<pkt_rgb_type>& s_in, hls::stream<pkt_rgb_type>& s_out)
{
#pragma HLS INTERFACE mode=ap_ctrl_none port=return
#pragma HLS INTERFACE mode=axis port=s_in
#pragma HLS INTERFACE mode=axis port=s_out
	while (true) {
		pixbuf pb { };
		loop_yuv422: while (true) {
#pragma HLS PIPELINE II=2
			pkt_rgb_type pkt0 { };
			pkt_rgb_type pkt1 { };
			s_in.read(pkt0);
			++pb.nzerobytes;
			if (!pkt0.last) {
				s_in.read(pkt1);
			}
			++pb.nzerobytes;
			bool in_last = pkt0.last | pkt1.last;
			u16 o0, o1;
			rgb_to_yuv422(pkt0.data, pkt1.data, o0, o1);
			out_yuv422(s_out, pb, pkt0.user, o0);
			out_yuv422(s_out, pb, pkt1.user, o1);
			if (in_last) {
				break;
			}
		}
		out_yuv422_rem(s_out, pb);
#if 0
		loop_yuv422_rem: while (true) {
#pragma HLS PIPELINE II=1
			out_yuv422_rem(s_out, pb);
			if (pb.nzerobytes == 0) {
				break;
			}
		}
#endif
	}
}

#endif

#if 0

void axis_rgb2yuv422(hls::stream<pkt_rgb_type>& s_in, hls::stream<pkt_yuv422_type>& s_out)
{
#pragma HLS INTERFACE mode=ap_ctrl_none port=return
#pragma HLS INTERFACE mode=axis port=s_in
#pragma HLS INTERFACE mode=axis port=s_out
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
}

#endif
