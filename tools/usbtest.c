/* rswdp.c
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

#include "usb.h"

int main(int argc, char **argv) {
	usb_handle *usb;
	char buf[4096];
	int n, sz = 64;

	if (argc > 1) {
		sz = atoi(argv[1]);
		if ((sz < 1) || (sz > 4096))
			return -1;
	}

	usb = usb_open(0x18d1, 0xdb01, 0);
	if (usb == 0) {
		fprintf(stderr, "cannot find device\n");
		return -1;
	}

	for (n = 0; n < sz; n++)
		buf[n] = n;

	usb_write(usb, buf, sz);
	return 0;
}

	

