/* debugger-commands.c
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
#include <ctype.h>
#include <stdarg.h>

#include <fcntl.h>
#include <sys/time.h>

/* TODO
 * - fault recovery (try dw 10000000 for example)
 */

#include <fw/types.h>
#include <protocol/rswdp.h>
#include "rswdp.h"

#include "debugger.h"

extern struct debugger_command debugger_commands[];

long long now() {
	struct timeval tv;
	gettimeofday(&tv, 0);
	return ((long long) tv.tv_usec) + ((long long) tv.tv_sec) * 1000000LL;
}

extern int disassemble_thumb2(u32 addr, u16 op0, u16 op1,
		char *text, int len) __attribute__ ((weak));

int disassemble(u32 addr) {
	char text[128];
	int r;
	union {
		u32 w[2];
		u16 h[4];
	} mem;

#if WITH_THUMB2_DISASSEMBLE
	if (!disassemble_thumb2)
		return -1;

	if (addr & 2) {
		if (swdp_ahb_read32(addr & (~3), mem.w, 2))
			return -1;
		r = disassemble_thumb2(addr, mem.h[1], mem.h[2], text, 128);
	} else {
		if (swdp_ahb_read32(addr & (~3), mem.w, 1))
			return -1;
		r = disassemble_thumb2(addr, mem.h[0], mem.h[1], text, 128);
	}
	if (r > 0)
		xprintf("%s\n", text);
	return r;
#else
	return -1;
#endif
}

int do_exit(int argc, param *argv) {
	exit(0);
	return 0;
}

int do_attach(int argc, param *argv) {
	return swdp_reset();
}

static u32 lastregs[19];

int do_regs(int argc, param *argv) {
	if (swdp_core_read_all(lastregs))
		return -1;

	xprintf("r0 %08x r4 %08x r8 %08x ip %08x psr %08x\n",
		lastregs[0], lastregs[4], lastregs[8],
		lastregs[12], lastregs[16]);
	xprintf("r1 %08x r5 %08x r9 %08x sp %08x msp %08x\n",
		lastregs[1], lastregs[5], lastregs[9],
		lastregs[13], lastregs[17]);
	xprintf("r2 %08x r6 %08x 10 %08x lr %08x psp %08x\n",
		lastregs[2], lastregs[6], lastregs[10],
		lastregs[14], lastregs[18]);
	xprintf("r3 %08x r7 %08x 11 %08x pc %08x\n",
		lastregs[3], lastregs[7], lastregs[11],
		lastregs[15]);
	disassemble(lastregs[15]);
	return 0;
}

int do_stop(int argc, param *argv) {
	swdp_core_halt();
	do_regs(0, 0);
	return 0;
}

int do_resume(int argc, param *argv) {
	swdp_core_resume();
	if (swdp_core_wait_for_halt() == 0)
		do_regs(0, 0);
	return 0;
}

int do_step(int argc, param *argv) {
	if (argc > 0) {
		u32 pc;
		do {
			swdp_core_step();
			swdp_core_wait_for_halt();
			if (swdp_core_read(15, &pc)) {
				xprintf("error\n");
				return -1;
			}
			xprintf(".");
			fflush(stdout);
		} while (pc != argv[0].n);
		xprintf("\n");
	} else {
		swdp_core_step();
		swdp_core_wait_for_halt();
	}
	do_regs(0, 0);
	return 0;
}

struct {
	const char *name;
	unsigned n;
} core_regmap[] = {
	{ "r0", 0 },
	{ "r1", 1 },
	{ "r2", 2 },
	{ "r3", 3 },
	{ "r4", 4 },
	{ "r5", 5 },
	{ "r6", 6 },
	{ "r7", 7 },
	{ "r8", 8 },
	{ "r9", 9 },
	{ "r10", 10 },
	{ "r11", 11 },
	{ "r12", 12 },
	{ "r13", 13 }, { "sp", 13 },
	{ "r14", 14 }, { "lr", 14 },
	{ "r15", 15 }, { "pc", 15 },
	{ "psr", 16 },
	{ "msp", 17 },
	{ "psp", 18 },
};

int read_memory_word(u32 addr, u32 *value) {
	return swdp_ahb_read(addr, value);
}

int read_register(const char *name, u32 *value) {
	int n;
	for (n = 0; n < (sizeof(core_regmap) / sizeof(core_regmap[0])); n++) {
		if (!strcasecmp(name, core_regmap[n].name)) {
			if (swdp_core_read(core_regmap[n].n, value))
				return -1;
			return 0;
		}
	}
	return ERROR_UNKNOWN;
}

int do_dr(int argc, param *argv) {
	unsigned n;
	u32 x = 0xeeeeeeee;
	if (argc < 1)
		return -1;
	for (n = 0; n < (sizeof(core_regmap) / sizeof(core_regmap[0])); n++) {
		if (!strcasecmp(argv[0].s, core_regmap[n].name)) {
			swdp_core_read(core_regmap[n].n, &x);
			xprintf("%s: %08x\n", argv[0].s, x);
			return 0;
		}
	}
	swdp_ahb_read(argv[0].n, &x);
	xprintf("%08x: %08x\n", argv[0].n, x);
	return 0;
}

int do_wr(int argc, param *argv) {
	unsigned n;
	if (argc < 2)
		return -1;
	for (n = 0; n < (sizeof(core_regmap) / sizeof(core_regmap[0])); n++) {
		if (!strcasecmp(argv[0].s, core_regmap[n].name)) {
			swdp_core_write(core_regmap[n].n, argv[1].n);
			xprintf("%s<<%08x\n", argv[0].s, argv[1].n);
			return 0;
		}
	}
	swdp_ahb_write(argv[0].n, argv[1].n);
	xprintf("%08x<<%08x\n", argv[0].n, argv[1].n);
	return 0;
}

int do_text(int argc, param *argv) {
	u8 data[1024], *x;
	u32 addr;

	if (argc < 1)
		return -1;
	addr = argv[0].n;
	memset(data, 0, sizeof(data));
	
	if (swdp_ahb_read32(addr, (void*) data, sizeof(data)/4))
		return -1;

	data[sizeof(data)-1] = 0;
	for (x = data; *x; x++) {
		if ((*x >= ' ') && (*x < 128))
			continue;
		if (*x == '\n')
			continue;
		*x = '.';
	}
	fprintf(stderr,"%s\n", (char*) data);
	return 0;
}

static u32 lastaddr = 0x20000000;
static u32 lastcount = 0x40;

int do_dw(int argc, param *argv) {
	u32 data[4096];
	u32 addr = lastaddr;
	u32 count = lastcount;
	unsigned n;

	if (argc > 0) addr = argv[0].n;
	if (argc > 1) count = argv[1].n;

	memset(data, 0xee, 256 *4);

	/* word align */
	addr = (addr + 3) & ~3;
	count = (count + 3) & ~3;
	if (count > sizeof(data))
		count = sizeof(data);

	lastaddr = addr + count;
	lastcount = count;

	count /= 4;
	if (swdp_ahb_read32(addr, data, count))
		return -1;
	for (n = 0; n < count; n++) {
		if ((n & 3) == 0)
			xprintf("\n%08x:", addr + (n << 2));
		xprintf(" %08x", data[n]);
	}
	xprintf("\n");
	return 0;
}


int do_db(int argc, param *argv) {
	u32 addr, count;
	u8 data[1024];
	unsigned n;

	if (argc < 2)
		return -1;

	addr = argv[0].n;
	count = argv[1].n;

	if (count > 1024)
		count = 1024;

	memset(data, 0xee, 1024);

	swdp_ahb_write(AHB_CSW, AHB_CSW_MDEBUG | AHB_CSW_PRIV |
		AHB_CSW_DBG_EN | AHB_CSW_8BIT);
	for (n = 0; n < count; n++) {
		u32 tmp;
		if (swdp_ahb_read(addr + n, &tmp)) {
			swdp_reset();
			break;
		}
		data[n] = tmp >> (8 * (n & 3));
	}

	swdp_ahb_write(AHB_CSW, AHB_CSW_MDEBUG | AHB_CSW_PRIV |
		AHB_CSW_DBG_EN | AHB_CSW_32BIT);

	for (n = 0; n < count; n++) {
		if ((n & 15) == 0)
			xprintf("\n%08x:", addr + n);
		xprintf(" %02x", data[n]);
	}
	xprintf("\n");
	return 0;
}

int do_download(int argc, param *argv) {
	u32 addr;
	u8 data[512*1024];
#if 0
	u8 vrfy[512*1024];
#endif
	int fd, r;

	long long t0, t1;

	if (argc != 2) {
		xprintf("error: usage: download <file> <addr>\n");
		return -1;
	}

	fd = open(argv[0].s, O_RDONLY);
	r = read(fd, data, sizeof(data));
	if ((fd < 0) || (r < 0)) {
		xprintf("error: cannot read '%s'\n", argv[0].s);
		return -1;
	}
	r = (r + 3) & ~3;
	addr = argv[1].n;

	xprintf("sending %d bytes...\n", r);
	t0 = now();
	if (swdp_ahb_write32(addr, (void*) data, r / 4)) {
		xprintf("error: failed to write data\n");
		return -1;
	}
	t1 = now();
	xprintf("%lld uS -> %lld B/s\n", (t1 - t0), 
		(((long long)r) * 1000000LL) / (t1 - t0));

#if 0
	t0 = now();
	if (swdp_ahb_read32(addr, (void*) vrfy, r / 4)) {
		xprintf("error: verify read failed\n");
		return -1;
	}
	t1 = now();
	xprintf("%lld uS. %lld B/s.\n", (t1 - t0), 
		(((long long)r) * 1000000LL) / (t1 - t0));
	if (memcmp(data,vrfy,r)) {
		xprintf("error: verify failed\n");
		return -1;
	}
#endif
	return 0;
}

int do_flash(int argc, param *argv) {
	u32 addr, xfer, size;
	u32 flash_buffer, flash_start, flash_size, flash_block;
	u8 data[512*1024], *p;
	int fd, r;

	if (argc != 2) {
		xprintf("error: usage: flash <file> <addr>\n");
		return -1;
	}

	if (debugger_variable("flash-buffer", &flash_buffer) ||
		debugger_variable("flash-start", &flash_start) ||
		debugger_variable("flash-size", &flash_size) ||
		debugger_variable("flash-block", &flash_block)) {
		xprintf("error: flash script missing or invalid\n");
		return -1;
	}

	addr = argv[1].n;

	if (addr & (flash_block - 1)) {
		xprintf("error: address unaligned with blocksize 0x%08x\n",
			flash_block);
		return -1;
	}
	if (addr < flash_start) {
		xprintf("error: start address less than flash start (0x%08x)\n",
			flash_start);
		return -1;
	}

	/* adjust available size based on offset */
	flash_size -= (addr - flash_start);
	
	memset(data, 0xff, sizeof(data));

	fd = open(argv[0].s, O_RDONLY);
	r = read(fd, data, sizeof(data));
	if ((fd < 0) || (r < 0)) {
		xprintf("error: cannot read '%s'\n", argv[0].s);
		return -1;
	}
	r = (r + 3) & ~3;
	size = r;
	if (size > flash_size) {
		xprintf("error: %d bytes does not fit in %d byte flash region\n",
			size, flash_size);
		return -1;
	}

	if (debugger_invoke("flash-setup", 0)) {
		xprintf("error: flash setup failed (@0x%08x)\n", addr);
		return -1;
	}
	p = data;
	while (size > 0) {
		xfer = (size > flash_block) ? flash_block : size;
		if (swdp_ahb_write32(flash_buffer, (void*) p, xfer / 4)) {
			xprintf("error: failed to write data\n");
			return -1;
		}
		if (debugger_invoke("flash-erase", 1, addr)) {
			xprintf("error: flash erase failed (@0x%08x)\n", addr);
			return -1;
		}
		if (debugger_invoke("flash-write", 1, addr)) {
			xprintf("error: flash write failed (@0x%08x)\n", addr);
			return -1;
		}
		addr += xfer;
		size -= xfer;
		p += xfer;
	}
	return 0;
}



int do_reset(int argc, param *argv) {
	swdp_core_halt();	
	swdp_ahb_write(CDBG_EMCR, 0);
	/* core reset and sys reset */
	swdp_ahb_write(0xe000ed0c, 0x05fa0005);
	swdp_ahb_write(CDBG_EMCR, 0);
#if 0
	if (argc > 0) {
		swdp_target_reset(argv[0].n);
	} else {
		swdp_target_reset(1);
		usleep(10000);
		swdp_target_reset(0);
	}
#endif
	return 0;
}

int do_reset_stop(int argc, param *argv) {
	swdp_core_halt();
	swdp_ahb_write(CDBG_EMCR, 1);
#if 1
	/* core reset and sys reset */
	swdp_ahb_write(0xe000ed0c, 0x05fa0005);
#else
	swdp_target_reset(1);
	usleep(10000);
	swdp_target_reset(0);
	usleep(10000);
#endif
	do_stop(0,0);
	swdp_ahb_write(CDBG_EMCR, 0);
	return 0;
}

int do_watch_pc(int argc, param *argv) {
	if (argc < 1)
		return -1;
	return swdp_watchpoint_pc(0, argv[0].n);
}

int do_watch_rw(int argc, param *argv) {
	if (argc < 1)
		return -1;
	return swdp_watchpoint_rw(0, argv[0].n);
}

int do_print(int argc, param *argv) {
	while (argc-- > 0)
		xprintf("%08x ", argv++[0].n);
	xprintf("\n");
	return 0;
}

int do_echo(int argc, param *argv) {
	while (argc-- > 0) {
		unsigned int argn = argv[0].n;
		const char *arg = argv++[0].s;

		if (arg[0] == '$') {
			xprintf("%08x ", argn);
		} else {
			xprintf("%s ", arg, argn);
		}
	}
	xprintf("\n");
	return 0;
}

int do_bootloader(int argc, param *argv) {
	return swdp_bootloader();
}

int do_setclock(int argc, param *argv) {
	if (argc < 1)
		return -1;
	return swdp_set_clock(argv[0].n);
}

int do_help(int argc, param *argv) {
	struct debugger_command *cmd;
	for (cmd = debugger_commands; cmd->func != NULL; cmd++) {
		xprintf("%-16s: %s\n", cmd->name, cmd->help);
	}

	return 0;
}

struct debugger_command debugger_commands[] = {
	{ "exit",	"", do_exit,	"" },
	{ "attach",	"", do_attach,	"attach/reattach to sw-dp" },
	{ "regs",	"", do_regs,	"show cpu registers" },
	{ "stop",	"", do_stop,	"halt cpu" },
	{ "step",	"", do_step,	"single-step cpu" },
	{ "go",		"", do_resume,	"resume cpu" },
	{ "dw",		"", do_dw,	"dump words" },
	{ "db",		"", do_db,	"dump bytes" },
	{ "dr",		"", do_dr,	"dump register" },
	{ "wr",		"", do_wr,	"write register" },
	{ "download",	"", do_download,"download file to device" },
	{ "flash",	"", do_flash,	"write file to device flash" },
	{ "reset",	"", do_reset,	"reset target" },
	{ "reset-stop",	"", do_reset_stop, "reset target and halt cpu" },
	{ "watch-pc",	"", do_watch_pc, "set watchpoint at addr" },
	{ "watch-rw",	"", do_watch_rw, "set watchpoint at addr" },
	{ "print",	"", do_print,	"print numeric arguments" },
	{ "echo",	"", do_echo, "echo command line" },
	{ "bootloader", "", do_bootloader, "reboot into bootloader" },
	{ "setclock",	"", do_setclock, "set clock rate (khz)" },
	{ "text",	"", do_text, "dump text" },
	{ "help",	"", do_help, "help" },
	{ 0, 0, 0, 0 },
};

