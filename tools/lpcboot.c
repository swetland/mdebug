/* lpcboot.c
 *
 * Copyright 2011 Brian Swetland <swetland@frotz.net>
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>

#include "usb.h"

int usage(void) {
	fprintf(stderr,
		"lpcboot usage\n-------------\n"
		" lpcboot query         display information about target\n"
		" lpcboot boot <app>    download app to ram and execute\n"
		" lpcboot flash <app>   write app image to flash\n"
		" lpcboot erase         erase app image\n"
		" lpcboot reboot        reboot the bootloader\n"
		" lpcboot app           reboot into the app in flash\n");
	return -1;
}

struct device_info {
	char part[16];
	char board[16];
	uint32_t version;
	uint32_t ram_base;
	uint32_t ram_size;
	uint32_t rom_base;
	uint32_t rom_size;
	uint32_t ununsed0;
	uint32_t ununsed1;
	uint32_t ununsed2;
};

#define DFU_DETACH      0
#define DFU_DNLOAD      1
#define DFU_UPLOAD      2
#define DFU_GETSTATUS   3
#define DFU_CLRSTATUS   4
#define DFU_GETSTATE    5
#define DFU_ABORT       6

#define STATE_APP_IDLE                  0x00
#define STATE_APP_DETACH                0x01
#define STATE_DFU_IDLE                  0x02
#define STATE_DFU_DOWNLOAD_SYNC         0x03
#define STATE_DFU_DOWNLOAD_BUSY         0x04
#define STATE_DFU_DOWNLOAD_IDLE         0x05
#define STATE_DFU_MANIFEST_SYNC         0x06
#define STATE_DFU_MANIFEST              0x07
#define STATE_DFU_MANIFEST_WAIT_RESET   0x08
#define STATE_DFU_UPLOAD_IDLE           0x09
#define STATE_DFU_ERROR                 0x0a

// 0xA1 getstatus: status[1] pollms[3] state[1] stridx[1]
int dfu_status(usb_handle *usb, unsigned *state) {
	uint8_t io[6];
	if (usb_ctrl(usb, io, 0xA1, DFU_GETSTATUS, 0, 0, 6) != 6) {
		fprintf(stderr, "dfu status: io error\n");
		return -1;
	}
	if (io[0]) {
		fprintf(stderr, "dfu status: 0x%02x\n", io[0]);
		return io[0];
	}
	*state = io[4];
	return 0;
}

static uint8_t lpcheader[16] = {
	0xda, 0xff, 0x40, 0x00, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

int dfu_download(void *_data, unsigned sz) {
	int once = 1;
	uint8_t *data = _data;
	usb_handle *usb;
	unsigned state;
	unsigned xfer;
	uint16_t count = 0;
	for (;;) {
		usb = usb_open(0x1fc9, 0x000c, 0);
		if (usb == 0) {
			if (once) {
				fprintf(stderr,"waiting for DFU device...\n");
				once = 0;
			}
		} else {
			break;
		}
	}
	if (dfu_status(usb, &state)) return -1;
	if (state != STATE_DFU_IDLE) {
		fprintf(stderr, "dfu state not idle (0x%02x)?\n", state);
		return -1;
	}

	// prepend isp header
	data -= 16;
	sz += 16;
	memcpy(data, lpcheader, 16);

	// send binary
	while (sz > 0) {
		fprintf(stderr,".");
		xfer = (sz > 2048) ? 2048 : sz;
		if (usb_ctrl(usb, data, 0x21, DFU_DNLOAD, count, 0, xfer) != xfer) {
			fprintf(stderr,"\ndfu dnload: io error\n");
			return -1;
		}
		count++;
		data += xfer;
		sz -= xfer;
		if (dfu_status(usb, &state)) return -1;
		if (state != STATE_DFU_DOWNLOAD_IDLE) {
			fprintf(stderr,"\ndfu dnload state 0x%02x?!\n", state);
			return -1;
		}
	}
	if (usb_ctrl(usb, NULL, 0x21, DFU_DNLOAD, 0, 0, 0) != 0) {
		fprintf(stderr,"\ndfu dnload2: io error\n");
		return -1;
	}
	fprintf(stderr,"\ndone\n");
	if (dfu_status(usb, &state)) return -1;
	if (state == STATE_DFU_MANIFEST_WAIT_RESET) return 0;
	fprintf(stderr,"dfu dnload2: state 0x%02x?!\n", state);
	return -1;
}

int main(int argc, char **argv) {
	usb_handle *usb;
	char buf[1024*1024+256];
	int fd, once = 1, sz = 0, dl = 0, dfu = 0;
	uint32_t cmd[3];
	uint32_t rep[2];
	struct device_info di;

	if (argc < 2) 
		return usage();

	cmd[0] = 0xDB00A5A5;
	if (!strcmp(argv[1],"flash")) {
		dl = 1;
		cmd[1] = 'W';
	} else if (!strcmp(argv[1],"flash:boot")) {
		dl = 1;
		cmd[1] = 'w';
	} else if (!strcmp(argv[1],"boot")) {
		dl = 1;
		cmd[1] = 'X';
	} else if (!strcmp(argv[1],"erase")) {
		cmd[1] = 'E';
	} else if (!strcmp(argv[1],"query")) {
		cmd[1] = 'Q';
	} else if (!strcmp(argv[1],"reboot")) {
		cmd[1] = 'R';
	} else if (!strcmp(argv[1],"app")) {
		cmd[1] = 'A';
	} else if (!strcmp(argv[1],"dfu")) {
		dl = 1;
		dfu = 1;
	} else {
		return usage();
	}

	if (dl) {
		if (argc < 3)
			return usage();
		fd = open(argv[2], O_RDONLY);
		sz = read(fd, buf + 256, sizeof(buf) - 256);
		close(fd);
		if ((fd < 0) || (sz < 1)) {
			fprintf(stderr,"error: cannot read '%s'\n", argv[2]);
			return -1;
		}
	}
	cmd[2] = sz;

	if (dfu) {
		return dfu_download(buf + 256, sz);
	}
	for (;;) {
		usb = usb_open(0x18d1, 0xdb00, 0);
		if (usb == 0) {
			if (once) {
				fprintf(stderr,"waiting for device...\n");
				once = 0;
			}
		} else {
			break;
		}
	}

	if (usb_write(usb, cmd, 12) != 12) {
		fprintf(stderr,"io error\n");
		return -1;
	}
	for (;;) {
		if (usb_read(usb, rep, 8) != 8) {
			fprintf(stderr,"io error\n");
			return -1;
		}
		if (rep[0] != 0xDB00A5A5) {
			fprintf(stderr,"protocol error\n");
			return -1;
		}
		if (rep[1] != 0) {
			fprintf(stderr,"%s failure\n", argv[1]);
			return -1;
		}
		if (!strcmp(argv[1],"query")) {
			if (usb_read(usb, &di, sizeof(di)) != sizeof(di)) {
				fprintf(stderr,"io error\n");
				return -1;
			}
			fprintf(stderr,"Part:  %s\n", di.part);
			fprintf(stderr,"Board: %s\n", di.board);
			fprintf(stderr,"RAM:   @%08x (%dKB)\n",
				di.ram_base, di.ram_size / 1024);
			fprintf(stderr,"Flash: @%08x (%dKB)\n",
				di.rom_base, di.rom_size / 1024);
			return 0;
		}
		if (!dl)
			break;
		fprintf(stderr,"sending %d bytes...\n", sz);
		if (usb_write(usb, buf + 256, sz) != sz) {
			fprintf(stderr,"download failure %d\n", sz);
			return -1;
		}
		dl = 0;
	}
	fprintf(stderr,"OKAY\n");
	return 0;
}
