#include "xparameters.h"
#include "xsdps.h"
#include "xil_printf.h"
#include "ff.h"
#include "xil_cache.h"
#include "xplatform_info.h"
#include "vcnet_sdcard.h"

static FIL fil;
static FATFS fatfs;
static char buffer[4096];

void vcnet_sdcard_init(void)
{
	TCHAR *path = "0:/";
	FRESULT res = f_mount(&fatfs, path, 0);
	if (res != FR_OK) {
		xil_printf("f_mount failed\r\n");
		return;
	}
	res = f_open(&fil, "vcnet.cnf", FA_READ);
	if (res != FR_OK) {
		xil_printf("file not found\r\n");
		goto cleanup;
	}
	res = f_lseek(&fil, 0);
	if (res != FR_OK) {
		xil_printf("f_lseek failed\r\n");
		goto cleanup;
	}
	UINT nread = 0;
	res = f_read(&fil, buffer, sizeof(buffer) - 1, &nread);
	if (res != FR_OK) {
		xil_printf("f_read failed\r\n");
		goto cleanup;
	}
	xil_printf("read %u bytes\r\n", (unsigned)nread);
cleanup:
	f_close(&fil);
	return;
}

char *
vcnet_sdcard_get_buffer(void)
{
	return buffer;
}
