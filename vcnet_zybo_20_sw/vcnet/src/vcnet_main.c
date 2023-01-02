
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

void platform_setup_timer_intr(void); // platform_zync.c

#define VCNET_BUFFER_PIX 1920

static u32 video_width = 1920;
static u32 video_height = 1080;
static int interlaced = 1;
static u32 cur_read_frame = 0;
static u8 *video_frame_data = NULL;
static u32 offset_line = 0;
static int waiting_flip = 0;
static u64 waiting_flip_time = 0;
static int send_stat = 0;
static char vcnet_stat_buffer[16];
static int reset_sys = 0;
static u64 prev_update_stat_time = 0;
static u32 status = 0;
static u32 reboot_status = 0;

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
		// memo: get_time_ms()Ç‡íxÇ≠Ç»ÇÈÇÃÇ≈íçà”
		vcnet_write32(0xf8000100, v | 0x03); // disable pll
	}
	vcnet_write32(0xf8000004, 0x767b); // lock SLCR
}

#if 0
static void vcnet_set_cpu_fast(void)
{
	vcnet_write32(0xf8000008, 0xdf0d); // unlock SLCR
	vcnet_write32(0xf8000100, v & ~0x03); // enable pll
	vcnet_write32(0xf8000004, 0x767b); // lock SLCR
}
#endif

static void vcnet_set_server_flags(void const *data, u32 datalen)
{
	if (datalen < 1) {
		return;
	}
	uint32_t val = 0;
	if (datalen > sizeof(val)) {
		datalen = sizeof(val);
	}
	memcpy(&val, data, datalen); // little endian
	{
		interlaced = (int)(val & 0x01);
	}
}

static void vcnet_recv_cb(u32 tag, void const *data, u32 datalen)
{
	if (verbose) {
		xil_printf("vcnet_recv_cb len=%u\r\n", (unsigned)datalen);
	}
	if (tag == 0x00) {
		send_stat = 1;
	} else if (tag == 0x01) {
		vcnet_command_add(data, datalen);
	} else if (tag == 0x02) {
		reset_sys = 1;
	} else if (tag == 0x03) {
		vcnet_ir_out(data, datalen); // blocks
	} else if (tag == 0x04) {
		vcnet_set_server_flags(data, datalen);
	}
}

static void vcnet_update_stat(void)
{
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

static void vcnet_exec_step_waiting_flip(void)
{
	u32 addr = *(volatile u32 *)(0x43c30000);
		// physical address where axi_vdma is writing
	u8 *p = (u8 *)addr;
	if (!(p >= video_frame_data && p < video_frame_data + 1920*1080*3)) {
		waiting_flip = 0;
		return;
	}
	u64 now = get_time_ms();
	if (now - waiting_flip_time > 1000) {
		if (verbose) {
			xil_printf("flip timeout\r\n");
		}
		waiting_flip = 0;
		return;
	}
}

static void vcnet_exec_step_send_stat_if(void)
{
	if (!send_stat) {
		return;
	}
	if (vcnet_net_send_buffer() < 16 + 8) {
		return;
	}
	u32 tag0 = 0;
	u32 tag1 = status;
	if (verbose > 2) {
		xil_printf("vcnet_send_data hb tag=%x,%x\r\n", tag0, tag1);
	}
	vcnet_net_send_data(tag0, tag1, &vcnet_stat_buffer[0], 16);
	send_stat = 0;
}

static void vcnet_exec_step_send_video(void)
{
	if (vcnet_net_send_buffer() < video_width * 3 + 8) {
		return;
	}
	if (video_width == 0) {
		video_frame_data = vcnet_video_get_frame_data(cur_read_frame, &video_width, &video_height);
		return;
	}
	u32 tag0 = 1; // video frame
	u32 last_flag = (offset_line + (interlaced ? 2 : 1) >= video_height) ? 0x80000000u : 0u;
	u32 interlaced_flag = (interlaced && (offset_line != 0 || cur_read_frame != 0)) ? 0x40000000u : 0u;
	u32 tag1 = (video_height << 16) | video_width | last_flag | interlaced_flag;
	void const *data = video_frame_data + offset_line * VCNET_BUFFER_PIX * 3;
	if (verbose > 2) {
		xil_printf("vcnet_send_data tag=%x,%x w=%u h=%u\r\n", tag0, tag1, video_width, video_height);
	}
	vcnet_net_send_data(tag0, tag1, data, video_width * 3);
	offset_line = (last_flag != 0) ? 0u : (offset_line + (interlaced ? 2 : 1));
	if (offset_line == 0) {
		cur_read_frame ^= 1u;
		if (interlaced && cur_read_frame) {
			offset_line = 1;
		}
		vcnet_video_set_video_frame(cur_read_frame ^ 1u, cur_read_frame);
		video_frame_data = vcnet_video_get_frame_data(cur_read_frame, &video_width, &video_height);
		waiting_flip = 1;
		waiting_flip_time = get_time_ms();
		return;
	}
}

static void vcnet_send_audio_range(char *start, char *finish)
{
	u32 tag0 = 2; // audio
	u32 tag1 = 0;
	vcnet_net_send_data(tag0, tag1, start, finish - start);
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
			xil_printf("skip lr %08x %08x %08x\r\n", v_prev_debug, v, audio_buffer_offset);
		}
		v_prev_debug = v;
		// break;
	}
	if (audio_buffer_offset < 1024) {
		return;
	}
	u32 sbsz = vcnet_net_send_buffer();
	if (sbsz < audio_buffer_offset * 2 + 32) {
		return;
	}
	u32 offset = audio_buffer_offset & ~1u;
	vcnet_send_audio_range((char *)audio_buffer, (char *)(audio_buffer + offset));
	if (verbose > 2) {
		xil_printf("iis send %u\r\n", audio_buffer_offset);
	}
	if (audio_buffer_offset != offset) {
		audio_buffer[0] = audio_buffer[audio_buffer_offset - 1];
	}
	audio_buffer_offset -= offset;
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
		if (t1 - t0 > 10) {
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
	if (waiting_flip) {
		check_slow(vcnet_exec_step_waiting_flip, "flip");
	} else {
		check_slow(vcnet_exec_step_send_video, "send");
	}
	check_slow(vcnet_send_audio, "audio");
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

static void vcnet_stop_cpu1(void)
{
#ifndef VCNET_DEBUG
	vcnet_write32(0xf8000008, 0xdf0d); // unlock SLCR
	vcnet_write32(0xf8000244, 0x20); // stop cpu1
	vcnet_write32(0xf8000004, 0x767b); // lock SLCR
#endif
}

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

int main(void)
{
	reboot_status = vcnet_read32(0xf8000258);
	vcnet_stop_cpu1();
	vcnet_measure_speed(5);
	vcnet_sdcard_init();
	vcnet_get_config("mac=", macaddr, sizeof(macaddr));
	vcnet_get_config("ip=", ipaddr, sizeof(ipaddr));
	verbose = 0;
#ifdef VCNET_DEBUG
	verbose = 1;
#endif
	xil_printf("vcnet main\r\n");
	init_platform();
	vcnet_video_init();
	vcnet_xvtc_workaround();
    vcnet_video_restart_video();
    vcnet_video_set_video_frame(1, 0);
	platform_setup_timer_intr(); // re-initialize timer intr handler
	platform_enable_interrupts();
    vcnet_net_init(macaddr, ipaddr);
    vcnet_net_set_recv_callback(&vcnet_recv_cb);
    video_frame_data = vcnet_video_get_frame_data(cur_read_frame, &video_width, &video_height);
    offset_line = 0;
#ifndef VCNET_DEBUG
	vcnet_iic_set_wd(10);
#endif
	vcnet_set_cpu_speed(0);
    while (1) {
    	vcnet_exec_step();
    	if (reset_sys) {
    		reset_sys = 0;
    		xil_printf("PSS_RST_CTRL\r\n");
    		sleep(30); // watchdog
//    		vcnet_reboot();
    	}
    }
	/* never reached */
	cleanup_platform();
	return 0;
}

