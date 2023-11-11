
// vim: ts=4:sw=4:noexpandtab

#include "xil_printf.h"
#include "sleep.h"
#include "platform.h"
#include "platform_config.h"
#include "xil_exception.h"
#include "vcnet_common.h"
#include "vcnet_command.h"
#include "vcnet_iic.h"
#include "vcnet_net.h"
#include "vcnet_video.h"
#include "vcnet_sdcard.h"

#include <stdlib.h>

void platform_setup_timer_intr(void); // platform_zync.c

#undef VCNET_DEBUG

#define VCNET_NUM_FRAME_BUFFERS 4
	// TODO: 4より大きくするとおかしくなるのはなぜ？ axi vdmaの設定では8枚。
#define VCNET_BUFFER_PIX 1920

static u32 video_width = 1920;
static u32 video_height = 1080;
static int interlaced = 1;
static u8 svr_flags_fmt = 0;
static u8 svr_flags_tcp_disable_video = 1;
static u8 svr_flags_tcp_disable_audio = 1;
static u8 svr_flags_udp_disable_video = 1;
static u8 svr_flags_udp_disable_audio = 1;
static u32 svr_flags_fps_limit_ms = 0;
static u32 video_crop_x_val = 0;
static u32 video_crop_x_offset = 0;
static u32 video_crop_x_size = 0;
static u32 read_frame_incl = 1;
static u32 cur_read_frame = 0;
static u32 cur_video_frame = 0;
static u32 cur_disp_frame = 0;
static u32 cur_odd_frame = 0;
static u8 *video_frame_data = NULL;
static u32 offset_line = 0;
static int waiting_flip = 0;
static u64 waiting_flip_time = 0;
static u64 prev_waiting_flip_time = 0;
static u64 prev_flip_time = 0;
static u8 send_stat_mask = 0; /* bit0: udp, bit1: tcp */
static char vcnet_stat_buffer[16];
static int reset_sys = 0;
static u64 prev_update_stat_time = 0;
static u32 status = 0;
static u32 reboot_status = 0;
static u64 stat_sendvideo_time_sum = 0;
static u64 stat_sendvideo_time_min = (uint64_t)-1;
static u64 stat_sendvideo_time_max = 0;
static u32 stat_sendvideo_time_count = 0;
static u64 stat_send_wouldblock = 0;
static u64 reboot_interval = 0;

static void vcnet_set_cpu_speed(int fast)
{
	static int cur_cpu_fast = 1;
	if (fast == cur_cpu_fast) {
		return;
	}
	cur_cpu_fast = fast;
	u32 v = vcnet_read32(0xf8000100);
	vcnet_write32(0xf8000008, 0xdf0d); // unlock SLCR
	if (fast) {
		vcnet_write32(0xf8000100, v & ~0x03); // enable pll
	} else {
		// memo: affects get_time_ms()
		vcnet_write32(0xf8000100, v | 0x03); // disable pll
	}
	vcnet_write32(0xf8000004, 0x767b); // lock SLCR
}

static void vcnet_recalc_video_crop(void)
{
	if (video_crop_x_val == 0) {
		video_crop_x_offset = 0;
		video_crop_x_size = video_width;
	} else {
		video_crop_x_offset = video_crop_x_val & 0xffffu;
		video_crop_x_size = video_crop_x_val >> 16;
		if (video_crop_x_offset > video_width) {
			video_crop_x_offset = video_width;
		}
		if (video_crop_x_offset + video_crop_x_size > video_width) {
			video_crop_x_size = video_width - video_crop_x_offset;
		}
	}
}

static void vcnet_update_video_size()
{
	vcnet_video_get_video_size(&video_width, &video_height);
	if (video_width == 0) {
		return;
	}
	if (svr_flags_fps_limit_ms != 0) {
		// TODO: fps_limit_ms設定はあまり正確でない。
		read_frame_incl = 1;
		return;
	}
	// TODO: この方法では60fpsと30fps程度以外に対応できない。もっとよい方法はないか。
	if (video_width >= 1920 || video_height >= 1080) {
		if (interlaced || svr_flags_fmt == 0) {
			read_frame_incl = 2;
		}
	} else {
		if (interlaced && svr_flags_fmt == 0) {
			read_frame_incl = 2;
		}
	}
}

#define IO_SVR_FLAGS_ADDR 0x43c30008

static void vcnet_set_server_flags(int tcpflag, void const *data, u32 datalen)
{
	if (datalen < 1) {
		return;
	}
	if (datalen >= 4) {
		uint32_t val = 0;
		memcpy(&val, data, 4); // little endian
		int interlaced_v = (int)(val & 0x01);
		if (interlaced != interlaced_v) {
			interlaced = interlaced_v;
			vcnet_update_video_size();
		}
		u8 svr_flags_fmt_v = (val >> 8) & 0xff;
		if (svr_flags_fmt != svr_flags_fmt_v) {
			svr_flags_fmt = svr_flags_fmt_v;
			*(volatile u32 *)(IO_SVR_FLAGS_ADDR) = (u32)svr_flags_fmt;
			xil_printf("vcnet_set_server_flags fmt=%u\r\n",
				(unsigned)svr_flags_fmt);
			vcnet_update_video_size();
		}
		xil_printf("vcnet_set_server_flags tcp=%d val=%x\r\n", tcpflag, val);
		if (tcpflag) {
			svr_flags_tcp_disable_video = (val >> 1) & 0x01;
			svr_flags_tcp_disable_audio = (val >> 2) & 0x01;
		} else {
			svr_flags_udp_disable_video = (val >> 1) & 0x01;
			svr_flags_udp_disable_audio = (val >> 2) & 0x01;
		}
		uint32_t fps_limit = (val >> 16) & 0xff;
		svr_flags_fps_limit_ms = (fps_limit != 0) ? 1000 / fps_limit : 0;
	}
	if (datalen >= 8) {
		memcpy(&video_crop_x_val, ((char const *)data) + 4, 4);
		vcnet_recalc_video_crop();
	}
}

#define IO_SPIBUF_ADDR 0x43c50000

static void vcnet_spi_buffer(void const *data, u32 datalen)
{
	if (datalen != 6) {
		xil_printf("vcnet_spi_buffer skip len=%u\r\n", (unsigned)datalen);
		return;
	}
	u8 const *const u8data = (u8 const *)data;
	u32 const offset = u8data[0];
	u32 const value = *((u32 const *)(u8data + 2));
	u32 const addr = IO_SPIBUF_ADDR + (offset * 4);
	if (verbose) {
		#if 0
		xil_printf("vcnet_spi_buffer addr=%x value=%x\r\n", (unsigned)addr,
			(unsigned)value);
		#endif
	}
	*((u32 volatile *)addr) = value;
}

static void vcnet_recv_cb(int tcpflag, u32 tag, void const *data, u32 datalen)
{
	if (verbose) {
		xil_printf("vcnet_recv_cb tcp=%d tag=%u len=%u\r\n", tcpflag,
			(unsigned)tag, (unsigned)datalen);
	}
	if (tag == 0x00) {
		send_stat_mask |= (1u << tcpflag);
	} else if (tag == 0x01) {
		vcnet_command_add(data, datalen);
	} else if (tag == 0x02) {
		reset_sys = 1;
	} else if (tag == 0x03) {
		vcnet_ir_out(data, datalen);
	} else if (tag == 0x04) {
		vcnet_set_server_flags(tcpflag, data, datalen);
	} else if (tag == 0x06) {
		vcnet_spi_buffer(data, datalen);
	} else if (tag == 0x09) {
		vcnet_gpio_out(data, datalen);
	}
	if (!tcpflag && tag == 0x00) {
		// echo back
		vcnet_net_send_raw_udp(tag, data, datalen);
	}
}

static void vcnet_update_stat(void)
{
	if (!vcnet_net_connected()) {
		svr_flags_tcp_disable_video = 1;
		svr_flags_tcp_disable_audio = 1;
		svr_flags_udp_disable_video = 1;
		svr_flags_udp_disable_audio = 1;
	}
	u64 now = get_time_ms();
	if (prev_update_stat_time + 5 > now) {
		return;
	}
	prev_update_stat_time = now;
	u32 value = 0;
	value |= ((now & 32) != 0) ? 0x01 : 0x00;
	value |= (vcnet_net_connected() != 0) ? 0x02 : 0x00;
	value |= (vcnet_video_locked() != 0) ? 0x04 : 0x00;
	value |= (vcnet_video_status() == 1) ? 0x08 : 0x00;
	value |= (reboot_status & 0x00ff0000);
	vcnet_iic_set_led(value);
	status = value;
	vcnet_set_cpu_speed(vcnet_net_connected());
}

static int vcnet_get_cur_dma_frame(u32 *offset_r)
{
	u32 addr = *(volatile u32 *)(0x43c30000);
		// physical address where axi_vdma is writing
	if (addr == 0) {
		if (offset_r) {
			*offset_r = 0;
		}
		return 0;
	}
	u8 *p = (u8 *)addr;
	for (int i = 0; i < VCNET_NUM_FRAME_BUFFERS; ++i) {
		u8 *const baseaddr = vcnet_video_get_frame_data(i);
		if (p >= baseaddr &&
			p < baseaddr + 1920 * 1080 * VCNET_BYTES_PER_PIXEL) {
			if (offset_r) {
				*offset_r = p - baseaddr;
			}
			return i;
		}
	}
	xil_printf("vcnet_get_cur_dma_frame: invalid addr %p\r\n", p);
	for (int i = 0; i < VCNET_NUM_FRAME_BUFFERS; ++i) {
		u8 *const baseaddr = vcnet_video_get_frame_data(i);
		xil_printf("%d %p\r\n", i, baseaddr);
	}
	abort();
	return 0;
}

static u32 next_frame(u32 v)
{
	if (++v >= VCNET_NUM_FRAME_BUFFERS) {
		v = 0;
	}
	return v;
}

static void vcnet_exec_step_flip(void)
{
	u64 now = get_time_ms();
	if (svr_flags_fps_limit_ms != 0) {
		if ((u32)(now - prev_flip_time) < svr_flags_fps_limit_ms) {
			return;
		}
	}
	u32 offset = 0;
	int dma_fr = vcnet_get_cur_dma_frame(&offset);
	// dmaアドレスが1080/32ライン先行するのを待ってからdma読み出し/表示フレーム
	// を切り替える
	if (dma_fr == cur_video_frame &&
		offset > (1920 * 1080 / 32) * VCNET_BYTES_PER_PIXEL) {
		if (waiting_flip == 1) {
			prev_flip_time = now;
		}
		u32 next_video_frame = next_frame(cur_video_frame);
		// ネット送信中フレームに追いついた場合はフレームを切り替えない
		if (next_video_frame != cur_read_frame) {
			cur_disp_frame = cur_video_frame;
			cur_video_frame = next_video_frame;
		}
		// レジスタにセットする。次のvsyncタイミングで反映される。
		vcnet_video_set_video_frame(cur_video_frame, cur_disp_frame);
		#if 0
		void *p = vcnet_video_get_frame_data(cur_disp_frame);
		Xil_DCacheInvalidateRange(p, 1920 * 1080 * VCNET_BYTES_PER_PIXEL);
		#endif
		#if 0
		xil_printf("flip cv=%u, cd=%u, cr=%u dma=%u\r\n", cur_video_frame,
			cur_disp_frame, cur_read_frame, dma_fr);
		#endif
		vcnet_update_video_size();
		vcnet_recalc_video_crop();
		if (waiting_flip == 1) {
			waiting_flip = 0;
		}
	}
	if (waiting_flip && now - waiting_flip_time > 1000) {
		if (verbose) {
			xil_printf("flip timeout\r\n");
		}
		waiting_flip = 0;
		prev_flip_time = now;
	}

}

static void vcnet_exec_step_send_stat_if(void)
{
	for (u8 i = 0; i < 2; ++i) {
		if ((send_stat_mask & (1 << i)) == 0) {
			continue;
		}
		if (i == 1 &&
			vcnet_net_get_sndbuf(i) < 32 + sizeof(vcnet_stat_buffer)) {
			continue;
		}
		if (i == 1) {
			u32 tag0 = 0;
			u32 tag1 = status;
			if (verbose > 2) {
				xil_printf("vcnet_send_data hb tag=%x,%x\r\n", tag0, tag1);
			}
			u32 hdmi_in_stat = vcnet_hdmi_in_stat_read();
			memcpy(&vcnet_stat_buffer[0], &hdmi_in_stat, 4);
			memcpy(&vcnet_stat_buffer[4], &stat_sendvideo_time_min, 4);
			memcpy(&vcnet_stat_buffer[8], &stat_sendvideo_time_min, 4);
			vcnet_net_send_tcp(tag0, tag1, &vcnet_stat_buffer[0], 12);
		} else {
		}
		send_stat_mask &= ~(1 << i);
	}
}

#define VCNET_PACKED_BITS_PER_PIXEL ((svr_flags_fmt == 0x02) ? 12 : 24)

static int get_video_tcpflag()
{
	if (svr_flags_tcp_disable_video == 0) {
		return 1;
	} else if (svr_flags_udp_disable_video == 0) {
		return 0;
	} else {
		return -1;
	}
}

static int get_audio_tcpflag()
{
	if (svr_flags_tcp_disable_audio == 0) {
		return 1;
	} else if (svr_flags_udp_disable_audio == 0) {
		return 0;
	} else {
		return -1;
	}
}

static void vcnet_exec_step_send_video()
{
	int tcpflag = get_video_tcpflag();
	if (tcpflag < 0) {
		return;
	}
	if (vcnet_net_get_sndbuf(tcpflag)
		< (video_width * VCNET_PACKED_BITS_PER_PIXEL / 8) + 32) {
		// 送信バッファに空きがないので送信しない。
		stat_send_wouldblock += 1;
		return;
	}
	if (video_width == 0) {
		video_frame_data = vcnet_video_get_frame_data(cur_read_frame);
		vcnet_update_video_size();
		return;
	}
	u32 tag0 = 1; // video frame
	u32 last_flag = (offset_line + (interlaced ? 2 : 1) >= video_height)
		? 0x80000000u : 0u;
		// last line of the current frame
	u32 interlaced_skipline =
		(interlaced && (offset_line != 0 || cur_odd_frame != 0))
		? 0x40000000u : 0u;
		// skip one line before drawing this line
	u32 yuv411_flag = (svr_flags_fmt == 0x02) ? 0x10000000u : 0;
	u32 tag1 = (video_height << 16) | video_crop_x_size | last_flag
		| interlaced_skipline | yuv411_flag;
	u8 const *data = video_frame_data +
		offset_line * VCNET_BUFFER_PIX * VCNET_BYTES_PER_PIXEL;
	u32 dma_offset = 0;
	int dma_fr = vcnet_get_cur_dma_frame(&dma_offset);
	{
		u32 roffset = offset_line * VCNET_BUFFER_PIX * VCNET_BYTES_PER_PIXEL;
		if (dma_fr == cur_read_frame && roffset + 1024 * 128 > dma_offset) {
			// dma書込みアドレスを追い越さないようにする。
			// TODO: この判定方法だとフレーム末尾で追い越す可能性がある。
			// TODO: この方法だとビデオフレームレートに追い付いていないときに書き込みアドレスとの追い越しが起きるはず。
			#if 0
			xil_printf("vcnet_exec_step_send_video: read offset overruns\r\n");
			#endif
			return;
		}
	}
	if (verbose > 2) {
		xil_printf("vcnet_exec_step_send_video tag=%x,%x w=%u h=%u\r\n",
			tag0, tag1, video_width, video_height);
	}
	{
		void const *frdata =
			data + (video_crop_x_offset * VCNET_PACKED_BITS_PER_PIXEL / 8);
		u32 frsz = (video_crop_x_size * VCNET_PACKED_BITS_PER_PIXEL / 8);
		if (tcpflag) {
			vcnet_net_send_tcp(tag0, tag1, frdata, frsz);
		} else {
			// udp送信。使用していない。
			u32 h0 = (offset_line << 16) | ((frsz + 8) & 0xffffu);
			u32 h1 = (2 << 24u); // vcnet_1g video
			h1 |= (tag1 >> 16);
			#if 0
			h1 |= (video_height & 0xffffu);
			if (interlaced_skipline) { h1 |= (1 << 22); }
			if (cur_odd_frame) { h1 |= (1 << 23); }
			if (svr_flags_fmt == 0x02) { h1 |= (1 << 20); } // yuv411
			#endif
			if (vcnet_net_send_large_udp(h0, h1, frdata, frsz) != 0) {
				return;
			}
			#if 0
			xil_printf("vcnet_exec_step_send_video udp %u\r\n", frsz); // FIXME
			#endif
		}
	}
	#if 0
	vcnet_net_flush_send_data();
	#endif
	offset_line = (last_flag != 0) ? 0u : (offset_line + (interlaced ? 2 : 1));
	if (offset_line == 0) {
		cur_odd_frame ^= 1u;
		if (interlaced && cur_odd_frame) {
			offset_line = 1;
		}
		waiting_flip = 1;
		waiting_flip_time = get_time_ms();
//		for (u32 i = 0; i < read_frame_incl; ++i) {
			if (++cur_read_frame >= VCNET_NUM_FRAME_BUFFERS) {
				cur_read_frame = 0;
			}
//		}
		video_frame_data = vcnet_video_get_frame_data(cur_read_frame);
		#if 0
		xil_printf("send vf cv=%u, cd=%u, cr=%u dma=%u\r\n", cur_video_frame,
			cur_disp_frame, cur_read_frame, dma_fr);
		#endif
		#if 0
		// prev_flip_timeはdmaアドレスが逃げるのを待った後の時刻。
		u64 tdiff = waiting_flip_time - prev_flip_time;
		#endif
		u64 tdiff = waiting_flip_time - prev_waiting_flip_time;
				prev_waiting_flip_time = waiting_flip_time;
		stat_sendvideo_time_min = stat_sendvideo_time_min > tdiff
			? tdiff : stat_sendvideo_time_min;
		stat_sendvideo_time_max = stat_sendvideo_time_max < tdiff
			? tdiff : stat_sendvideo_time_max;
		stat_sendvideo_time_sum += tdiff;
		stat_sendvideo_time_count += 1;
		if (stat_sendvideo_time_count >= 300) {
			xil_printf("vcnet_send_data sendvideo_time=%u %u %u %u %u\r\n",
				(unsigned)stat_sendvideo_time_sum,
				(unsigned)stat_sendvideo_time_min,
				(unsigned)stat_sendvideo_time_max,
				(unsigned)stat_send_wouldblock,
				(unsigned)stat_sendvideo_time_count);
			stat_sendvideo_time_min = (uint64_t)-1;
			stat_sendvideo_time_max = 0;
			stat_sendvideo_time_sum = 0;
			stat_sendvideo_time_count = 0;
			stat_send_wouldblock = 0;
			#if 0
			xil_printf("dbg_lwip_get_mem_count %u %u\r\n",
				dbg_lwip_get_mem_count(), dbg_lwip_get_memp_count());
			#endif
		}
	}
}

static void vcnet_exec_step_disp()
{
	u32 w = 0;
	u32 h = 0;
	if (vcnet_video_locked()) {
		w = video_width;
		h = video_height;
	}
	vcnet_video_restart_disp_if(w, h);
}

static void vcnet_send_audio_range(int tcpflag, char *start, char *finish)
{
	if (tcpflag) {
		u32 tag0 = 2; // audio
		u32 tag1 = 0;
		vcnet_net_send_tcp(tag0, tag1, start, finish - start);
		vcnet_net_flush_tcp();
	} else {
		u32 h0 = (finish - start) + 8;
		u32 h1 = (1 << 24u); // audio
		vcnet_net_send_large_udp(h0, h1, start, finish - start);
	}
}

#define AUDIO_BUFFER_SIZE 4096
static u16 audio_buffer[AUDIO_BUFFER_SIZE];
static u32 audio_buffer_offset = 0;
static u32 v_prev_debug = 0;

static void vcnet_send_audio()
{
	if (!vcnet_net_connected()) {
		audio_buffer_offset = 0;
		return;
	}
	int tcpflag = get_audio_tcpflag();
	if (tcpflag < 0) {
		return;
	}
	while (audio_buffer_offset < 2048) {
		u32 v = vcnet_iis_read();
		if (v == 0) {
			break;
		}
		/* L: even offset, R: odd offset */
		if ((audio_buffer_offset & 1) == ((v >> 7) & 1)) {
			audio_buffer[audio_buffer_offset] = v >> 16;
			++audio_buffer_offset;
		} else {
			xil_printf("skip lr %08x %08x %08x\r\n", v_prev_debug, v,
				audio_buffer_offset);
		}
		v_prev_debug = v;
		// break;
	}
	if (audio_buffer_offset < 1024) {
		return;
	}
	u32 sbsz = vcnet_net_get_sndbuf(tcpflag);
	if (sbsz < audio_buffer_offset * 2 + 32) {
		return;
	}
	u32 offset = audio_buffer_offset & ~1u; // align to 2byte boundary
	vcnet_send_audio_range(tcpflag, (char *)audio_buffer,
		(char *)(audio_buffer + offset));
		// FIXME: この下でaudio_bufferを書き換えるので、コピーする必要が
		// あるのでは?
	if (verbose > 2) {
		xil_printf("iis send %u\r\n", audio_buffer_offset);
	}
	if (audio_buffer_offset != offset) {
		audio_buffer[0] = audio_buffer[audio_buffer_offset - 1]; // odd byte
	}
	audio_buffer_offset -= offset; // 0 or 1
}

static void check_slow(void (*f)(void), const char *s)
{
	u64 t0 = 0;
	u64 t1 = 0;
	if (verbose) {
		t0 = get_time_ms();
	}
	(*f)();
	if (verbose) {
		t1 = get_time_ms();
		if (t1 - t0 > 3) {
			xil_printf("slow func %s\r\n", s);
		}
	}
}

static void vcnet_exec_step(void)
{
	check_slow(vcnet_video_exec_step, "video");
	check_slow(vcnet_net_exec_step, "net");
	check_slow(vcnet_command_exec_step, "command");
	check_slow(vcnet_exec_step_send_stat_if, "stat");
	for (int i = 0; i < 2; ++i) {
		check_slow(vcnet_exec_step_send_video, "send");
	}
	check_slow(vcnet_send_audio, "audio");
	check_slow(vcnet_net_flush_tcp, "flushsend");
	check_slow(vcnet_exec_step_flip, "flip");
	check_slow(vcnet_exec_step_disp, "disp");
	vcnet_update_stat();
}

static void vcnet_reboot()
{
	vcnet_write32(0xf8000008, 0xdf0d);
	vcnet_write32(0xf8000200, 0x01);
}

static void vcnet_data_abort_handler(void *CallBackRef)
{
	vcnet_reboot();
}

void vcnet_xvtc_workaround()
{
	// workaround for the problem that xvtc causes DataAbort
	// when HDMI cable is unplugged.
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_DATA_ABORT_INT,
		vcnet_data_abort_handler, NULL);
}

static uint32_t rand_seed = 0;

static void init_short(uint32_t seed)
{
	rand_seed = seed;
}

static uint32_t generate()
{
	rand_seed = 1103515245u * rand_seed + 12345u;
	return rand_seed;
}

static void vcnet_measure_speed(int n)
{
	uint64_t loop_max = 1000u;
	for (int k = 0; k < n; ++k) {
		loop_max *= 10u;
		unsigned long t0 = get_time_ms();
		init_short(12345u);
		for (uint64_t i = 0u; i < loop_max; ++i) {
			// rand_seed = 1103515245u * rand_seed + 12345u;
			generate();
		}
		uint32_t v = generate();
		unsigned long t1 = get_time_ms();
		xil_printf("%lu %x %lu\r\n", (unsigned long)loop_max,
			(unsigned)v, t1 - t0);
	}
}

#if 0
static void vcnet_stop_cpu1(void)
{
	#ifndef VCNET_DEBUG
	// CPU1を止める。ただしデバッガが使えなくなるのでデバッグ時は何もしない。
	vcnet_write32(0xf8000008, 0xdf0d); // unlock SLCR
	vcnet_write32(0xf8000244, 0x20); // stop cpu1
	vcnet_write32(0xf8000004, 0x767b); // lock SLCR
	#endif
}
#endif

static void vcnet_get_config(const char *key, char *buffer, size_t buffersz)
{
	const char *const fbuf = vcnet_sdcard_get_buffer();
	size_t keylen = strlen(key);
	const char *p = NULL;
	for (p = fbuf; *p; ++p) {
		if (strncmp(p, key, keylen) == 0) {
			break;
		}
	}
	if (*p == 0) {
		return;
	}
	p += keylen;
	size_t s = 0;
	for (s = 0; p[s]; ++s) {
		if (p[s] < 0x20) {
			break;
		}
	}
	if (s > buffersz - 1) {
		s = buffersz - 1;
	}
	memcpy(buffer, p, s);
	buffer[s] = 0;
}

static char macaddr[32];
static char ipaddr[16];
static char rebootint[32];

static u64
read_u64(const char *p)
{
	u64 r = 0;
	while (*p) {
		if (*p >= '0' && *p <= '9') {
			u8 v = *p - '0';
			r *= 10;
			r += v;
			++p;
		} else {
			break;
		}
	}
	return r;
}

int main(void)
{
	reboot_status = vcnet_read32(0xf8000258);
//	vcnet_stop_cpu1();
	vcnet_measure_speed(4);
	vcnet_sdcard_init();
	vcnet_get_config("mac=", macaddr, sizeof(macaddr));
	vcnet_get_config("ip=", ipaddr, sizeof(ipaddr));
	vcnet_get_config("reboot_interval=", rebootint, sizeof(rebootint));
	xil_printf("s=%s\r\n", rebootint);
	reboot_interval = read_u64(rebootint) * 1000;
	xil_printf("reboot_interval=%u msec\r\n", (unsigned)reboot_interval);
	verbose = 0;
	#ifdef VCNET_DEBUG
	verbose = 0; // FIXME
	#endif
	xil_printf("vcnet main\r\n");
	init_platform();
	vcnet_video_init();
	vcnet_xvtc_workaround();
	vcnet_video_start_disp(0);
	vcnet_video_set_video_frame(1, 0);
	platform_setup_timer_intr(); // re-initialize timer intr handler
	platform_enable_interrupts();
	vcnet_net_init(macaddr, ipaddr);
	vcnet_net_set_recv_callback(&vcnet_recv_cb);
	video_frame_data = vcnet_video_get_frame_data(cur_read_frame);
	vcnet_update_video_size();
	offset_line = 0;
	#ifndef VCNET_DEBUG
	vcnet_iic_set_wd(10);
	#endif
	vcnet_set_cpu_speed(0);
	while (1) {
		vcnet_exec_step();
		if (!vcnet_net_is_link_up()) {
			xil_printf("vcnet link down\r\n");
			reset_sys = 1;
		}
		if (reboot_interval != 0 && vcnet_net_connected() == 0) {
			u64 now = get_time_ms();
			u64 last_busy = vcnet_net_get_last_busy_time();
			if (now - last_busy > reboot_interval) {
				xil_printf("vcnet idle\r\n");
				vcnet_reboot();
			}
		}
		if (reset_sys) {
			reset_sys = 0;
			xil_printf("vcnet sleep 30 wdt\r\n");
			sleep(30); // watchdog timer expires
			#if 0
			vcnet_reboot();
			#endif
		}
	}
	/* never reached */
	cleanup_platform();
	return 0;
}

