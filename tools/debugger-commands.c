/* debugger-commands.);
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

#include <fw/types.h>
#include <protocol/rswdp.h>
#include "rswdp.h"
#include "arm-v7m.h"

#include "debugger.h"
#include "lkdebug.h"

#define _AGENT_HOST_ 1
#include <agent/flash.h>

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
		xprintf(XDATA, "%s\n", text);
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

	xprintf(XDATA, "r0 %08x r4 %08x r8 %08x ip %08x psr %08x\n",
		lastregs[0], lastregs[4], lastregs[8],
		lastregs[12], lastregs[16]);
	xprintf(XDATA, "r1 %08x r5 %08x r9 %08x sp %08x msp %08x\n",
		lastregs[1], lastregs[5], lastregs[9],
		lastregs[13], lastregs[17]);
	xprintf(XDATA, "r2 %08x r6 %08x 10 %08x lr %08x psp %08x\n",
		lastregs[2], lastregs[6], lastregs[10],
		lastregs[14], lastregs[18]);
	xprintf(XDATA, "r3 %08x r7 %08x 11 %08x pc %08x\n",
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
	return 0;
}

int do_step(int argc, param *argv) {
	if (argc > 0) {
		u32 pc;
		do {
			swdp_core_step();
			swdp_core_wait_for_halt();
			if (swdp_core_read(15, &pc)) {
				xprintf(XCORE, "step: error\n");
				return -1;
			}
			fflush(stdout);
		} while (pc != argv[0].n);
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
			xprintf(XDATA, "%s: %08x\n", argv[0].s, x);
			return 0;
		}
	}
	swdp_ahb_read(argv[0].n, &x);
	xprintf(XDATA, "%08x: %08x\n", argv[0].n, x);
	return 0;
}

int do_wr(int argc, param *argv) {
	unsigned n;
	if (argc < 2)
		return -1;
	for (n = 0; n < (sizeof(core_regmap) / sizeof(core_regmap[0])); n++) {
		if (!strcasecmp(argv[0].s, core_regmap[n].name)) {
			swdp_core_write(core_regmap[n].n, argv[1].n);
			xprintf(XDATA, "%s<<%08x\n", argv[0].s, argv[1].n);
			return 0;
		}
	}
	swdp_ahb_write(argv[0].n, argv[1].n);
	xprintf(XDATA, "%08x<<%08x\n", argv[0].n, argv[1].n);
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
	xprintf(XDATA, "%08x: %s\n", addr, (char*) data);
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

	for (n = 0; count > 0; n += 4, addr += 16) {
		switch (count) {
		case 1:
			count = 0;
			xprintf(XDATA, "%08x: %08x\n", addr, data[n]);
			break;
		case 2:
			count = 0;
			xprintf(XDATA, "%08x: %08x %08x\n",
				addr, data[n], data[n+1]);
			break;
		case 3:
			count = 0;
			xprintf(XDATA, "%08x: %08x %08x %08x\n",
				addr, data[n], data[n+1], data[n+2]);
			break;
		default:
			count -= 4;
			xprintf(XDATA, "%08x: %08x %08x %08x %08x\n",
				addr, data[n], data[n+1], data[n+2], data[n+3]);
			break;
		}
	}
	return 0;
}


int do_db(int argc, param *argv) {
	u32 addr, count;
	u8 data[1024];
	char line[256];
	unsigned n, m, xfer;

	if (argc < 2)
		return -1;

	addr = argv[0].n;
	count = argv[1].n;

	if (count > 1024)
		count = 1024;

	memset(data, 0xee, 1024);
	// todo: fix this
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

	for (n = 0; count > 0; count -= xfer) {
		xfer = (count > 16) ? 16 : count;
		char *p = line + sprintf(line, "%08x:", addr + n);
		for (m = 0; m < xfer; m++) {
			p += sprintf(p, " %02x", data[n++]);
		}
		xprintf(XDATA, "%s\n", line);
	}
	return 0;
}

// vector catch flags to apply
u32 vcflags = DEMCR_VC_HARDERR | DEMCR_VC_BUSERR | DEMCR_VC_STATERR | DEMCR_VC_CHKERR;

int do_reset(int argc, param *argv) {
	swdp_core_halt();
	swdp_ahb_write(DEMCR, DEMCR_TRCENA | vcflags);
	/* core reset and sys reset */
	swdp_ahb_write(0xe000ed0c, 0x05fa0005);
	swdp_ahb_write(DEMCR, DEMCR_TRCENA | vcflags);
	return 0;
}

int do_reset_hw(int argc, param *argv) {
	swdp_target_reset(1);
	usleep(10000);
	swdp_target_reset(0);
	usleep(10000);
	return 0;
}

int do_reset_stop(int argc, param *argv) {
	swdp_core_halt();
	// enable vector-trap on reset, enable DWT/FPB
	swdp_ahb_write(DEMCR, DEMCR_VC_CORERESET | DEMCR_TRCENA | vcflags);
	// core reset and sys reset
	swdp_ahb_write(0xe000ed0c, 0x05fa0005);
	//swdp_core_wait_for_halt();
	do_stop(0,0);
	swdp_ahb_write(DEMCR, DEMCR_TRCENA | vcflags);
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

int do_watch_off(int argc, param *argv) {
	return swdp_watchpoint_disable(0);
}

int do_print(int argc, param *argv) {
	while (argc-- > 0)
		xprintf(XCORE, "%08x\n", argv++[0].n);
	return 0;
}

int do_echo(int argc, param *argv) {
	while (argc-- > 0) {
		unsigned int argn = argv[0].n;
		const char *arg = argv++[0].s;

		if (arg[0] == '$') {
			xprintf(XCORE, "%08x\n", argn);
		} else {
			xprintf(XCORE, "%s\n", arg);
		}
	}
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

int do_swoclock(int argc, param *argv) {
	if (argc < 1)
		return -1;
	return swo_set_clock(argv[0].n);
}

int do_help(int argc, param *argv) {
	struct debugger_command *cmd;
	for (cmd = debugger_commands; cmd->func != NULL; cmd++) {
		xprintf(XCORE, "%-16s: %s\n", cmd->name, cmd->help);
	}

	return 0;
}

void *get_builtin_file(const char *fn, size_t *sz);

void *load_file(const char *fn, size_t *_sz) {
	int fd;
	off_t sz;
	void *data = NULL;
	fd = open(fn, O_RDONLY);
	if (fd < 0) goto fail;
	sz = lseek(fd, 0, SEEK_END);
	if (sz < 0) goto fail;
	if (lseek(fd, 0, SEEK_SET)) goto fail;
	if ((data = malloc(sz + 4)) == NULL) goto fail;
	if (read(fd, data, sz) != sz) goto fail;
	*_sz = sz;
	return data;
fail:
	if (data) free(data);
	if (fd >= 0) close(fd);
	return NULL;
}

int do_download(int argc, param *argv) {
	u32 addr;
	void *data;
	size_t sz;
	long long t0, t1;

	if (argc != 2) {
		xprintf(XCORE, "error: usage: download <file> <addr>\n");
		return -1;
	}

	if ((data = load_file(argv[0].s, &sz)) == NULL) {
		xprintf(XCORE, "error: cannot read '%s'\n", argv[0].s);
		return -1;
	}
	sz = (sz + 3) & ~3;
	addr = argv[1].n;

	xprintf(XCORE, "sending %d bytes...\n", sz);
	t0 = now();
	if (swdp_ahb_write32(addr, (void*) data, sz / 4)) {
		xprintf(XCORE, "error: failed to write data\n");
		free(data);
		return -1;
	}
	t1 = now();
	xprintf(XCORE, "%lld uS -> %lld B/s\n", (t1 - t0), 
		(((long long)sz) * 1000000LL) / (t1 - t0));
	free(data);
	return 0;
}

int do_run(int argc, param *argv) {
	u32 addr;
	void *data;
	size_t sz;
	u32 sp, pc;
	if (argc != 2) {
		xprintf(XCORE, "error: usage: run <file> <addr>\n");
		return -1;
	}
	if ((data = load_file(argv[0].s, &sz)) == NULL) {
		xprintf(XCORE, "error: cannot read '%s'\n", argv[0].s);
		return -1;
	}
	swdp_core_halt();
	sz = (sz + 3) & ~3;
	addr = argv[1].n;
	if (swdp_ahb_write32(addr, (void*) data, sz / 4)) {
		xprintf(XCORE, "error: failed to write data\n");
		free(data);
		return -1;
	}
	memcpy(&sp, data, 4);
	memcpy(&pc, ((char*) data) + 4, 4);
	swdp_core_write(13, sp);
	swdp_core_write(15, pc);
	swdp_ahb_write(0xe000ed0c, 0x05fa0002);
	swdp_core_write(16, 0x01000000);
	swdp_core_resume();
	free(data);
	return 0;
}


void *load_agent(const char *arch, size_t *_sz) {
	void *data;
	size_t sz;
	char name[256];
	if (arch == NULL) return NULL;
	snprintf(name, 256, "agent-%s.bin", arch);
	if ((data = get_builtin_file(name, &sz))) {
		void *copy = malloc(sz + 4);
		if (copy == NULL) return NULL;
		memcpy(copy, data, sz);
		*_sz = sz;
		return copy;
	}
	snprintf(name, sizeof(name), "out/agent-%s.bin", arch);
	return load_file(name, _sz);
}

static char *agent_arch = NULL;

int do_setarch(int argc, param *argv) {
	char *x;
	if (argc != 1) return -1;
	if((x = strdup(argv[0].s))) {
		free(agent_arch);
		agent_arch = x;
	}
	return 0;
}

int invoke(u32 agent, u32 func, u32 r0, u32 r1, u32 r2, u32 r3) {
	swdp_core_write(0, r0);
	swdp_core_write(1, r1);
	swdp_core_write(2, r2);
	swdp_core_write(3, r3);
	swdp_core_write(13, agent - 4);
	swdp_core_write(14, agent | 1); // include T bit
	swdp_core_write(15, func | 1); // include T bit

	// if the target has bogus data at 0, the processor may be in
	// pending-exception state after reset-stop, so we will clear
	// any exceptions and then set the PSR to something reasonable

	// Write VECTCLRACTIVE to AIRCR
	swdp_ahb_write(0xe000ed0c, 0x05fa0002);
	swdp_core_write(16, 0x01000000);

	// todo: readback and verify?

	xprintf(XCORE, "invoke <func@%08x>(0x%x,0x%x,0x%x,0x%x)\n", func, r0, r1, r2, r3);

	swdp_core_resume();
	if (swdp_core_wait_for_halt() == 0) {
		// todo: timeout after a few seconds?
		u32 pc = 0xffffffff, res = 0xffffffff;
		swdp_core_read(0, &res);
		swdp_core_read(15, &pc);
		if (pc != agent) {
			xprintf(XCORE, "error: pc (%08x) is not at %08x\n", pc, agent);
			return -1;
		}
		if (res) xprintf(XCORE, "failure code %08x\n", res);
		return res;
	}
	xprintf(XCORE, "interrupted\n");
	return -1;
}

int run_flash_agent(u32 flashaddr, void *data, size_t data_sz) {
	flash_agent *agent = NULL;
	size_t agent_sz;

	if ((agent = load_agent(agent_arch, &agent_sz)) == NULL) {
		xprintf(XCORE, "error: cannot load flash agent for architecture '%s'\n",
			agent_arch ? agent_arch : "unknown");
		xprintf(XCORE, "error: set architecture with: arch <name>\n");
		goto fail;
	}
	// sanity check
	if ((agent_sz < sizeof(flash_agent)) ||
		(agent->magic != AGENT_MAGIC) ||
		(agent->version != AGENT_VERSION)) {
		xprintf(XCORE, "error: invalid agent image\n");
		goto fail;
	}
	// replace magic with bkpt instructions
	agent->magic = 0xbe00be00;

	if (do_attach(0,0)) {
		xprintf(XCORE, "error: failed to attach\n");
		goto fail;
	}
	do_reset_stop(0,0);

	if (agent->flags & FLAG_BOOT_ROM_HACK) {
		xprintf(XCORE, "executing boot rom\n");
		if (swdp_watchpoint_rw(0, 0)) {
			goto fail;
		}
		swdp_core_resume();
		swdp_core_wait_for_halt();
		swdp_watchpoint_disable(0);
		// todo: timeout?
		// todo: confirm halted
	}

	if (swdp_ahb_write32(agent->load_addr, (void*) agent, agent_sz / 4)) {
		xprintf(XCORE, "error: failed to download agent\n");
		goto fail;
	}
	if (invoke(agent->load_addr, agent->setup, 0, 0, 0, 0)) {
		goto fail;
	}
	if (swdp_ahb_read32(agent->load_addr + 16, (void*) &agent->data_addr, 4)) {
		goto fail;
	}
	xprintf(XCORE, "agent %d @%08x, buffer %dK @%08x, flash %dK @%08x\n",
		agent_sz, agent->load_addr,
		agent->data_size / 1024, agent->data_addr,
		agent->flash_size / 1024, agent->flash_addr);

	if ((flashaddr == 0) && (data == NULL) && (data_sz == 0xFFFFFFFF)) {
		// erase all
		flashaddr = agent->flash_addr;
		data_sz = agent->flash_size;
	}

	if ((flashaddr < agent->flash_addr) ||
		(data_sz > agent->flash_size) ||
		((flashaddr + data_sz) > (agent->flash_addr + agent->flash_size))) {
		xprintf(XCORE, "invalid flash address %08x\n", flashaddr);
		goto fail;
	}

	if (data == NULL) {
		// erase
		if (invoke(agent->load_addr, agent->erase, flashaddr, data_sz, 0, 0)) {
			xprintf(XCORE, "failed to erase %d bytes at %08x\n", data_sz, flashaddr);
			goto fail;
		}
	} else {
		// write
		u8 *ptr = (void*) data;
		u32 xfer;
		xprintf(XCORE, "flashing %d bytes at %08x...\n", data_sz, flashaddr);
		if (invoke(agent->load_addr, agent->erase, flashaddr, data_sz, 0, 0)) {
			xprintf(XCORE, "failed to erase %d bytes at %08x\n", data_sz, flashaddr);
			goto fail;
		}
		while (data_sz > 0) {
			if (data_sz > agent->data_size) {
				xfer = agent->data_size;
			} else {
				xfer = data_sz;
			}
			if (swdp_ahb_write32(agent->data_addr, (void*) ptr, xfer / 4)) {
				xprintf(XCORE, "download to %08x failed\n", agent->data_addr);
				goto fail;
			}
			if (invoke(agent->load_addr, agent->write,
				flashaddr, agent->data_addr, xfer, 0)) {
				xprintf(XCORE, "failed to flash %d bytes to %08x\n", xfer, flashaddr);
				goto fail;
			}
			ptr += xfer;
			data_sz -= xfer;
			flashaddr += xfer;
		}
	}

	free(agent);
	if (data) free(data);
	return 0;
fail:
	if (agent) free(agent);
	if (data) free(data);
	return -1;
}

int do_flash(int argc, param *argv) {
	void *data = NULL;
	size_t data_sz;
	if (argc != 2) {
		xprintf(XCORE, "error: usage: flash <file> <addr>\n");
		return -1;
	}
	if ((data = load_file(argv[0].s, &data_sz)) == NULL) {
		xprintf(XCORE, "error: cannot load '%s'\n", argv[0].s);
		return -1;
	}
	// word align
	data_sz = (data_sz + 3) & ~3;
	return run_flash_agent(argv[1].n, data, data_sz);
}

int do_erase(int argc, param *argv) {
	if ((argc == 1) && !strcmp(argv[0].s, "all")) {
		return run_flash_agent(0, NULL, 0xFFFFFFFF);
	}
	if (argc != 2) {
		xprintf(XCORE, "error: usage: erase <addr> <length> | erase all\n");
		return -1;
	}
	return run_flash_agent(argv[0].n, NULL, argv[1].n);
}

int do_log(int argc, param *argv) {
	unsigned flags = 0;
	while (argc > 0) {
		if (!strcmp(argv[0].s, "gdb")) {
			flags |= LF_GDB;
		} else if (!strcmp(argv[0].s, "swd")) {
			flags |= LF_SWD;
		} else {
			xprintf(XCORE, "error: allowed flags: gdb swd\n");
			return -1;
		}
		argc--;
		argv++;
	}
	log_flags = flags;
	return 0;
}

int do_finfo(int argc, param *argv) {
	u32 cfsr = 0, hfsr = 0, dfsr = 0, mmfar = 0, bfar = 0;
	swdp_ahb_read(CFSR, &cfsr);
	swdp_ahb_read(HFSR, &hfsr);
	swdp_ahb_read(DFSR, &dfsr);
	swdp_ahb_read(MMFAR, &mmfar);
	swdp_ahb_read(BFAR, &bfar);

	xprintf(XDATA, "CFSR %08x  MMFAR %08x\n", cfsr, mmfar);
	xprintf(XDATA, "HFSR %08x   BFAR %08x\n", hfsr, bfar);
	xprintf(XDATA, "DFSR %08x\n", dfsr);

	if (cfsr & CFSR_IACCVIOL)	xprintf(XDATA, ">MM: Inst Access Violation\n");
	if (cfsr & CFSR_DACCVIOL)	xprintf(XDATA, ">MM: Data Access Violation\n");
	if (cfsr & CFSR_MUNSTKERR)	xprintf(XDATA, ">MM: Derived MM Fault on Exception Return\n");
	if (cfsr & CFSR_MSTKERR)	xprintf(XDATA, ">MM: Derived MM Fault on Exception Entry\n");
	if (cfsr & CFSR_MLSPERR)	xprintf(XDATA, ">MM: MM Fault During Lazy FP Save\n");
	if (cfsr & CFSR_MMARVALID)	xprintf(XDATA, ">MM: MMFAR has valid contents\n");

	if (cfsr & CFSR_IBUSERR)	xprintf(XDATA, ">BF: Bus Fault on Instruction Prefetch\n");
	if (cfsr & CFSR_PRECISERR)	xprintf(XDATA, ">BF: Precise Data Access Error, Addr in BFAR\n");
	if (cfsr & CFSR_IMPRECISERR)	xprintf(XDATA, ">BF: Imprecise Data Access Error\n");
	if (cfsr & CFSR_UNSTKERR)	xprintf(XDATA, ">BF: Derived Bus Fault on Exception Return\n");
	if (cfsr & CFSR_STKERR)		xprintf(XDATA, ">BF: Derived Bus Fault on Exception Entry\n");
	if (cfsr & CFSR_LSPERR)		xprintf(XDATA, ">BF: Bus Fault During Lazy FP Save\n");
	if (cfsr & CFSR_BFARVALID)	xprintf(XDATA, ">BF: BFAR has valid contents\n");

	if (cfsr & CFSR_UNDEFINSTR)	xprintf(XDATA, ">UF: Undefined Instruction Usage Fault\n");
	if (cfsr & CFSR_INVSTATE)	xprintf(XDATA, ">UF: EPSR.T or ESPR.IT invalid\n");
	if (cfsr & CFSR_INVPC)		xprintf(XDATA, ">UF: Integrity Check Error on EXC_RETURN\n");
	if (cfsr & CFSR_NOCP)		xprintf(XDATA, ">UF: Coprocessor Error\n");
	if (cfsr & CFSR_UNALIGNED)	xprintf(XDATA, ">UF: Unaligned Access Error\n");
	if (cfsr & CFSR_DIVBYZERO)	xprintf(XDATA, ">UF: Divide by Zero\n");

	if (hfsr & HFSR_VECTTBL)	xprintf(XDATA, ">HF: Vector Table Read Fault\n");
	if (hfsr & HFSR_FORCED)		xprintf(XDATA, ">HF: Exception Escalated to Hard Fault\n");
	if (hfsr & HFSR_DEBUGEVT)	xprintf(XDATA, ">HF: Debug Event\n");

	// clear sticky fault bits
	swdp_ahb_write(CFSR, CFSR_ALL);
	swdp_ahb_write(HFSR, HFSR_ALL);
	return 0;
}

extern int swdp_step_no_ints;

int do_maskints(int argc, param *argv) {
	if (argc != 1) {
		xprintf(XCORE, "usage: maskints [on|off|always]\n");
		return -1;
	}
	if (!strcmp(argv[0].s, "on")) {
		swdp_step_no_ints = 1;
		xprintf(XCORE, "maskints: while stepping\n");
	} else if (!strcmp(argv[0].s, "always")) {
		swdp_step_no_ints = 2;
		xprintf(XCORE, "maskints: always\n");
	} else {
		swdp_step_no_ints = 0;
		xprintf(XCORE, "maskints: never\n");
	}
	return 0;
}

int do_threads(int argc, param *argv) {
	if (argc == 1) {
		if (strcmp(argv[0].s, "clear")) {
			xprintf(XCORE, "usage: threads [clear]\n");
			return -1;
		}
		swdp_core_halt();
		clear_lk_threads();
		return 0;
	}
	lkthread_t *t;
	swdp_core_halt();
	t = find_lk_threads(1);
	dump_lk_threads(t);
	free_lk_threads(t);
	return 0;
}

int remote_msg(u32 cmd) {
	unsigned timeout = 250;
	u32 n = 0;
	if (swdp_ahb_write(DCRDR, cmd)) return -1;
	if (swdp_ahb_read(DEMCR, &n)) return -1;
	if (swdp_ahb_write(DEMCR, n | DEMCR_MON_PEND)) return -1;
	while (timeout > 0) {
		if (swdp_ahb_read(DCRDR, &n)) return -1;
		if (!(n & 0x80000000)) return 0;
		timeout--;
	}
	return -1;
}

int do_wconsole(int argc, param *argv) {
	if (argc != 1) return -1;
	const char *line = argv[0].s;
	while (*line) {
		if (remote_msg(0x80000000 | *line)) goto oops;
		line++;
	}
	if (remote_msg(0x80000000 | '\n')) goto oops;
	return 0;
oops:
	xprintf(XCORE, "console write failed\n");
	return -1;
}


struct debugger_command debugger_commands[] = {
	{ "exit",	"", do_exit,		"" },
	{ "attach",	"", do_attach,		"attach/reattach to sw-dp" },
	{ "regs",	"", do_regs,		"show cpu registers" },
	{ "finfo",	"", do_finfo,		"Fault Information" },
	{ "stop",	"", do_stop,		"halt cpu" },
	{ "step",	"", do_step,		"single-step cpu" },
	{ "go",		"", do_resume,		"resume cpu" },
	{ "dw",		"", do_dw,		"dump words" },
	{ "db",		"", do_db,		"dump bytes" },
	{ "dr",		"", do_dr,		"dump register" },
	{ "wr",		"", do_wr,		"write register" },
	{ "download",	"", do_download,	"download file to device" },
	{ "run",	"", do_run,		"download file and execute it" },
	{ "flash",	"", do_flash,		"write file to device flash" },
	{ "erase",	"", do_erase,		"erase flash" },
	{ "reset",	"", do_reset,		"reset target" },
	{ "reset-stop",	"", do_reset_stop,	"reset target and halt cpu" },
	{ "reset-hw",	"", do_reset_hw,	"strobe /RESET pin" },
	{ "watch-pc",	"", do_watch_pc,	"set watchpoint at addr" },
	{ "watch-rw",	"", do_watch_rw,	"set watchpoint at addr" },
	{ "watch-off",	"", do_watch_off,	"disable watchpoint" },
	{ "log",	"", do_log,		"enable/disable logging" },
	{ "maskints",	"", do_maskints,	"enable/disable IRQ mask during step" },
	{ "print",	"", do_print,		"print numeric arguments" },
	{ "echo",	"", do_echo,		"echo command line" },
	{ "bootloader", "", do_bootloader,	"reboot into bootloader" },
	{ "setclock",	"", do_setclock,	"set SWD clock rate (khz)" },
	{ "swoclock",	"", do_swoclock,	"set SWO clock rate (khz)" },
	{ "arch",	"", do_setarch,		"set architecture for flash agent" },
	{ "threads",	"", do_threads,		"thread dump" },
	{ "text",	"", do_text,		"dump text" },
	{ "wconsole",	"", do_wconsole,	"write to remote console" },
	{ "help",	"", do_help,		"help" },
	{ 0, 0, 0, 0 },
};

