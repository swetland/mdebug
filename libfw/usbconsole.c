/* usbconsole.c
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

#include <fw/types.h>
#include <fw/lib.h>

#include <arch/cpu.h>

#define UC_OUT_SZ 256
#define UC_IN_SZ 32

struct usbconsole {
	volatile u32 out_head;
	volatile u32 out_tail;
	volatile u32 out_busy;	
	volatile u32 in_count;
	u8 *out_data;
//	u8 *in_data;
};

/* buffers separate from main structure to save on flash size */
static u8 uc_out_buffer[UC_OUT_SZ];
//static u8 uc_in_buffer[UC_IN_SZ];

static struct usbconsole the_usb_console = {
	.out_head = 0,
	.out_tail = 0,
	.out_busy = 1,
	.in_count = 0,
	.out_data = uc_out_buffer,
//	.in_data = uc_in_buffer,
};

static void uc_write(struct usbconsole *uc) {
	u32 pos, len, rem;

	pos = uc->out_tail & (UC_OUT_SZ - 1);
	len = uc->out_head - uc->out_tail;
	rem = UC_OUT_SZ - pos;
	
	if (len > rem)
		len = rem;
	if (len > 64)
		len = 64;

	disable_interrupts();
	uc->out_busy = 1;
	usb_ep1_write(uc->out_data + pos, len);
	enable_interrupts();
	uc->out_tail += len;
}

static void uc_putc(unsigned c) {
	struct usbconsole *uc = &the_usb_console;
	while ((uc->out_head - uc->out_tail) == UC_OUT_SZ) {
		if (!uc->out_busy)
			uc_write(uc);
	}
	uc->out_data[uc->out_head & (UC_OUT_SZ - 1)] = c;
	uc->out_head++;
}

void printu(const char *fmt, ...) {
	struct usbconsole *uc = &the_usb_console;
        va_list ap;
        va_start(ap, fmt);
        vprintx(uc_putc, fmt, ap);
        va_end(ap);
	if ((uc->out_head - uc->out_tail) && (!uc->out_busy)) {
		uc_write(uc);
	}
}

void handle_ep1_tx_empty(void) {
	struct usbconsole *uc = &the_usb_console;
	uc->out_busy = 0;
	if (uc->out_head - uc->out_tail)
		uc_write(uc);
}

void (*usb_console_rx_cb)(u8 *buf, int len);

void handle_ep1_rx_full(void) {
//	struct usbconsole *uc = &the_usb_console;
	u8 buf[65];
	int len = usb_ep1_read(buf, 64);

	if (usb_console_rx_cb)
		usb_console_rx_cb(buf, len);
}

void handle_usb_online(int yes) {
	if (yes) {
		handle_ep1_tx_empty();
	} else {
		the_usb_console.out_busy = 1;
	}
}

void usb_console_init(void) {
	usb_ep1_tx_empty_cb = handle_ep1_tx_empty;
	usb_ep1_rx_full_cb = handle_ep1_rx_full;
	usb_online_cb = handle_usb_online;
//	usb_init(0x18d1, 0xdb05, 0, 0);
	usb_unmask_ep1_tx_empty();
	usb_unmask_ep1_rx_full();
}

