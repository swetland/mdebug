/* arm-m-debug
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
#include <unistd.h>

#include <fw/types.h>
#include "debugger.h"
#include "rswdp.h"

#include "arm-v7m.h"

// CDBG_* comes from here right now -- FIXME
#include <protocol/rswdp.h>

static volatile int ATTN;

void swdp_interrupt(void) {
	ATTN++;
	if (write(2, "\b\b*INTERRUPT*\n", 16)) { /* do nothing */ }
}


int swdp_core_write(u32 n, u32 v) {
	if (mem_wr_32(CDBG_REG_DATA, v)) {
		return -1;
	}
	if (mem_wr_32(CDBG_REG_ADDR, (n & 0x1F) | 0x10000)) {
		return -1;
	}
	return 0;
}

int swdp_core_read(u32 n, u32 *v) {
	if (mem_wr_32(CDBG_REG_ADDR, n & 0x1F)) {
		return -1;
	}
	if (mem_rd_32(CDBG_REG_DATA, v)) {
		return -1;
	}
	return 0;
}

int swdp_core_read_all(u32 *v) {
	unsigned n;
	for (n = 0; n < 19; n++) {
		if (mem_wr_32(CDBG_REG_ADDR, n & 0x1F)) {
			return -1;
		}
		if (mem_rd_32(CDBG_REG_DATA, v++)) {
			return -1;
		}
	}
	return 0;
}

int swdp_step_no_ints = 0;

int swdp_core_halt(void) {
	u32 x;
	if (mem_rd_32(CDBG_CSR, &x)) return -1;
	x &= (CDBG_C_HALT | CDBG_C_DEBUGEN | CDBG_C_MASKINTS);
	x |= CDBG_CSR_KEY | CDBG_C_DEBUGEN | CDBG_C_HALT;
	return mem_wr_32(CDBG_CSR, x);
}

int swdp_core_step(void) {
	u32 x;
	if (mem_rd_32(CDBG_CSR, &x)) return -1;
	x &= (CDBG_C_HALT | CDBG_C_DEBUGEN | CDBG_C_MASKINTS);
	x |= CDBG_CSR_KEY;

	if (!(x & CDBG_C_HALT)) {
		// HALT if we're not already HALTED
		x |= CDBG_C_HALT | CDBG_C_DEBUGEN;
		mem_wr_32(CDBG_CSR, x);
	}
	if (swdp_step_no_ints) {
		// set MASKINTS if not already set
		if (!(x & CDBG_C_MASKINTS)) {
			x |= CDBG_C_MASKINTS;
			mem_wr_32(CDBG_CSR, x);
		}
	} else {
		// clear MASKINTs if not already clear
		if (x & CDBG_C_MASKINTS) {
			x &= (~CDBG_C_MASKINTS);
			mem_wr_32(CDBG_CSR, x);
		}
	}
	// STEP
	x &= (~CDBG_C_HALT);
	return mem_wr_32(CDBG_CSR, x | CDBG_C_STEP);
}

int swdp_core_resume(void) {
	u32 x;
	if (mem_rd_32(CDBG_CSR, &x)) return -1;
	x &= (CDBG_C_HALT | CDBG_C_DEBUGEN | CDBG_C_MASKINTS);
	x |= CDBG_CSR_KEY | CDBG_C_DEBUGEN;

	if (swdp_step_no_ints > 1) {
		// not just on during step, but always
		if (!(x & CDBG_C_MASKINTS)) {
			x |= CDBG_C_MASKINTS;
			mem_wr_32(CDBG_CSR, x);
		}
	} else {
		if (x & CDBG_C_MASKINTS) {
			x &= (~CDBG_C_MASKINTS);
			mem_wr_32(CDBG_CSR, x);
		}
	}

	x &= ~(CDBG_C_HALT | CDBG_C_STEP);
	return mem_wr_32(CDBG_CSR, x);
}

int swdp_core_wait_for_halt(void) {
	int last = ATTN;
	u32 csr;
	for (;;) {
		if (mem_rd_32(CDBG_CSR, &csr))
			return -1;
		if (csr & CDBG_S_HALT)
			return 0;
		if (ATTN != last)
			return -2;
	}
}

int swdp_ahb_wait_for_change(u32 addr, u32 oldval) {
	int last = ATTN;
	u32 val;
	do {
		if (mem_rd_32(addr, &val))
			return -1;
		if (ATTN != last)
			return -2;
	} while (val == oldval);
	return 0;
}

int swdp_watchpoint(unsigned n, u32 addr, u32 func) {
	int r;
	if (n > 3)
		return -1;

	/* enable DWT, enable all exception traps */
	r = mem_wr_32(DEMCR, DEMCR_TRCENA | DEMCR_VC_CORERESET);
	r |= mem_wr_32(DWT_FUNC(n), DWT_FN_DISABLED);
	if (func != DWT_FN_DISABLED) {
		r |= mem_wr_32(DWT_COMP(n), addr);
		r |= mem_wr_32(DWT_MASK(n), 0);
		r |= mem_wr_32(DWT_FUNC(n), func);
	}
	return r;
}

int swdp_watchpoint_pc(unsigned n, u32 addr) {
	return swdp_watchpoint(n, addr, DWT_FN_WATCH_PC);
}

int swdp_watchpoint_rd(unsigned n, u32 addr) {
	return swdp_watchpoint(n, addr, DWT_FN_WATCH_RD);
}

int swdp_watchpoint_wr(unsigned n, u32 addr) {
	return swdp_watchpoint(n, addr, DWT_FN_WATCH_WR);
}

int swdp_watchpoint_rw(unsigned n, u32 addr) {
	return swdp_watchpoint(n, addr, DWT_FN_WATCH_RW);
}

int swdp_watchpoint_disable(unsigned n) {
	return swdp_watchpoint(n, 0, DWT_FN_DISABLED);
}

