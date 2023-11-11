
// vim: ts=4:sw=4:noexpandtab

#include "xparameters.h"
#include "xsdps.h"
#include "xil_printf.h"
#include "ff.h"
#include "xil_cache.h"
#include "xplatform_info.h"
#include "vcnet_sdcard.h"

static FATFS fatfs;
static char buffer[4096];
static FIL fil_conf;

void vcnet_sdcard_init(void)
{
	TCHAR *path = "0:/";
	FRESULT res = f_mount(&fatfs, path, 0);
	if (res != FR_OK) {
		xil_printf("f_mount failed\r\n");
		return;
	}
	res = f_open(&fil_conf, "vcnet.cnf", FA_READ);
	if (res != FR_OK) {
		xil_printf("file not found\r\n");
		goto cleanup;
	}
	res = f_lseek(&fil_conf, 0);
	if (res != FR_OK) {
		xil_printf("f_lseek failed\r\n");
		goto cleanup;
	}
	char *bufp = buffer;
	UINT blen = sizeof(buffer) - 1;
	while (blen > 0) {
		UINT nread = 0;
		res = f_read(&fil_conf, bufp, blen, &nread);
		if (res != FR_OK) {
			xil_printf("f_read failed\r\n");
			break;
		}
		bufp += nread;
		blen -= nread;
		xil_printf("read %u bytes\r\n", (unsigned)nread);
		if (nread == 0) {
			break;
		}
	}
	#if 0
	xil_printf("read: [%s]\r\n", buffer);
	#endif
cleanup:
	f_close(&fil_conf);
	return;
}

char *
vcnet_sdcard_get_buffer(void)
{
	return buffer;
}
