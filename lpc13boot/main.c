/* main.c
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
#include <fw/io.h>
#include <fw/string.h>

#include <arch/hardware.h>

extern u8 board_name[];

#define RAM_BASE	0x10000000
#define RAM_SIZE	(7 * 1024)

#define ROM_BASE	0x00001000
#define ROM_SIZE	(28 * 1024)

void (*romcall)(u32 *in, u32 *out) = (void*) 0x1fff1ff1;

/* { PREPARE, startpage, endpage } */
#define OP_PREPARE	50
/* { WRITE, dstaddr, srcaddr, bytes, sysclk_khz } */
#define OP_WRITE	51
/* { ERASE, startpage, endpage, sysclk_khz } */
#define OP_ERASE	52

struct device_info {
	u8 part[16];
	u8 board[16];
	u32 version;
	u32 ram_base;
	u32 ram_size;
	u32 rom_base;
	u32 rom_size;
	u32 unused0;
	u32 unused1;
	u32 unused2;
};

struct device_info DEVICE = {
	.part = "LPC1343",
	.version = 0x0001000,
	.ram_base = RAM_BASE,
	.ram_size = RAM_SIZE,
	.rom_base = ROM_BASE,
	.rom_size = ROM_SIZE,
};

void iap_erase_page(unsigned addr) {
	u32 in[4];
	u32 out[5];
	in[0] = OP_PREPARE;
	in[1] = (addr >> 12) & 0xF;
	in[2] = (addr >> 12) & 0xF;
	romcall(in, out);
	in[0] = OP_ERASE;
	in[1] = (addr >> 12) & 0xF;
	in[2] = (addr >> 12) & 0xF;
	in[3] = 48000;
	romcall(in, out);
}

void iap_write_page(unsigned addr, void *src) {
	u32 in[5];
	u32 out[5];
	in[0] = OP_PREPARE;
	in[1] = (addr >> 12) & 0xF;
	in[2] = (addr >> 12) & 0xF;
	romcall(in, out);
	in[0] = OP_WRITE;
	in[1] = addr;
	in[2] = (u32) src;
	in[3] = 0x1000;
	in[4] = 48000;
	romcall(in, out);
}

static u32 buf[64];

void start_image(u32 pc, u32 sp);

void _boot_image(void *img) {
	start_image(((u32*)img)[1], ((u32*)img)[0]);
}

void boot_image(void *img) {
	board_debug_led(0);
	usb_stop();

	/* TODO: shut down various peripherals */
	_boot_image(img);
}

void handle(u32 magic, u32 cmd, u32 arg) {
	u32 reply[2];
	u32 addr, xfer;
	void *ram = (void*) RAM_BASE;

	if (magic != 0xDB00A5A5)
		return;

	reply[0] = magic;
	reply[1] = -1;

	switch (cmd) {
	case 'E':
		iap_erase_page(0x1000);
		reply[1] = 0;
		break;
	case 'W':
		if (arg > ROM_SIZE)
			break;
		reply[1] = 0;
		usb_xmit(reply, 8);
		addr = ROM_BASE;
		while (arg > 0) {
			xfer = (arg > 4096) ? 4096 : arg;
			usb_recv(ram, xfer);
			iap_erase_page(addr);
			iap_write_page(addr, ram);
			addr += 4096;
			arg -= xfer;
		}
		break;
	case 'X':
		if (arg > RAM_SIZE)
			break;
		reply[1] = 0;
		usb_xmit(reply, 8);
		usb_recv(ram, arg);
		usb_xmit(reply, 8);

		/* let last txn clear */
		usb_recv_timeout(buf, 64, 10);

		boot_image(ram);
		break;
	case 'Q':
		reply[1] = 0;
		usb_xmit(reply, 8);
		usb_xmit(&DEVICE, sizeof(DEVICE));
		return;
	case 'A':
		/* reboot-into-app -- signal to bootloader via GPREGn */
		writel(0x12345678, GPREG0);
		writel(0xA5A50001, GPREG1);
	case 'R':
		/* reboot "normally" */
		reply[1] = 0;
		usb_xmit(reply, 8);
		usb_recv_timeout(buf, 64, 10);
		reboot();
	default:
		break;
	}
	usb_xmit(reply, 8);
}

int main() {
	int n, x, timeout;
	u32 tmp;
	u32 gpr0,gpr1;

	/* sample GPREG and clear */
	gpr0 = readl(GPREG0);
	gpr1 = readl(GPREG1);
	writel(0xBBBBBBBB, GPREG0);
	writel(0xBBBBBBBB, GPREG1);

	/* request to boot directly into the "app" image */
	if ((gpr0 == 0x12345678) && (gpr1 == 0xA5A50001))
		_boot_image((void*) 0x1000);

	board_init();

	usb_init(0x18d1, 0xdb00, "m3dev", "m3boot");

	/* check for an app image and set a 3s timeout if it exists */
	tmp = *((u32*) 0x1000);
	if ((tmp != 0) && (tmp != 0xFFFFFFFF))
		timeout = 30;
	else
		timeout = 0;

	/* request to stay in the bootloader forever? */
	if ((gpr0 == 0x12345678) && (gpr1 == 0xA5A50000))
		timeout = 0;

	strcpy((char*) DEVICE.board, (char*) board_name);

	if (timeout) {
		/* wait up to 1s to enumerate */
		writel(readl(SYS_CLK_CTRL) | SYS_CLK_CT32B0, SYS_CLK_CTRL);
		writel(48, TM32B0PR);
		writel(TM32TCR_ENABLE, TM32B0TCR);
		while (!usb_online() && (readl(TM32B0TC) < 2000000)) ;
		writel(readl(SYS_CLK_CTRL) & (~SYS_CLK_CT32B0), SYS_CLK_CTRL);
		if (!usb_online())
			goto start_app;
	}

	x = 0;
	for (;;) {
		board_debug_led(x & 1);
		x++;
		n = usb_recv_timeout(buf, 64, 100);

		if ((n == -ETIMEOUT) && timeout) {
			timeout--;
			if (timeout == 0)
				break;
		}

		if (n == 12) {
			timeout = 0;
			handle(buf[0], buf[1], buf[2]);
		}
	}

start_app:
	/* warm reset into the app */
	writel(0x12345678, GPREG0);
	writel(0xA5A50001, GPREG1);
	reboot();

	return 0;
}

