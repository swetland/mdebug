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

int main(int argc, char **argv) {
	usb_handle *usb;
	char buf[32768];
	int fd, once = 1, sz = 0, dl = 0;
	uint32_t cmd[3];
	uint32_t rep[2];
	struct device_info di;

	if (argc < 2) 
		return usage();

	cmd[0] = 0xDB00A5A5;
	if (!strcmp(argv[1],"flash")) {
		dl = 1;
		cmd[1] = 'W';
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
	} else {
		return usage();
	}

	if (dl) {
		if (argc < 3)
			return usage();
		fd = open(argv[2], O_RDONLY);
		sz = read(fd, buf, sizeof(buf));
		close(fd);
		if ((fd < 0) || (sz < 1)) {
			fprintf(stderr,"error: cannot read '%s'\n", argv[2]);
			return -1;
		}
	}
	cmd[2] = sz;

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
		if (usb_write(usb, buf, sz) != sz) {
			fprintf(stderr,"download failure %d\n", sz);
			return -1;
		}
		dl = 0;
	}
	fprintf(stderr,"OKAY\n");
	return 0;
}
