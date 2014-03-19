/* uconsole.c
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

int main(int argc, char **argv) {
	int r, once;
	usb_handle *usb;
	char buf[64];

	once = 1;
	for (;;) {
		usb = usb_open(0x18d1, 0xdb05, 0);
		if (usb == 0) {
			usb = usb_open(0x18d1, 0xdb04, 1);
		}
		if (usb == 0) {
			if (once) {
				fprintf(stderr,"waiting for device...\n");
				once = 0;
			}
		} else {
			break;
		}
	}

	fprintf(stderr,"connected\n");

	for (;;) {
		r = usb_read(usb, buf, 64);
		if (r < 0)
			break;
		if (write(1, buf, r)) { /* do nothing */ }
	}

	return 0;
}
