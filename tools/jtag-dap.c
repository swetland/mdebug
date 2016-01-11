/* jtag-dap.c
 *
 * Copyright 2015 Brian Swetland <swetland@frotz.net>
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
#include <string.h>

#include <fw/types.h>
#include "debugger.h"
#include "jtag.h"
#include "dap-registers.h"

#define CSW_ERRORS (DPCSW_STICKYERR | DPCSW_STICKYCMP | DPCSW_STICKYORUN)
#define CSW_ENABLES (DPCSW_CSYSPWRUPREQ | DPCSW_CDBGPWRUPREQ | DPCSW_ORUNDETECT)

typedef struct DAP {
	jtag_txn *jtag;
	u32 cached_ir;
	u32 ir_pre, ir_post;
	u32 dr_pre, dr_post;
} DAP;

void dap_init_jtag(DAP *dap) {
	jtag_txn_init(dap->jtag);
	dap->cached_ir = 0xFFFFFFFF;
	dap->jtag->ir_pre = dap->ir_pre;
	dap->jtag->ir_post = dap->ir_post;
	dap->jtag->dr_pre = dap->dr_pre;
	dap->jtag->dr_post = dap->dr_post;
	dap->jtag->state = JTAG_IDLE;
}

void dap_init(DAP *dap, jtag_txn *t, u32 ir_pre, u32 ir_post, u32 dr_pre, u32 dr_post) {
	dap->jtag = t;
	dap->ir_pre = ir_pre;
	dap->ir_post = ir_post;
	dap->dr_pre = dr_pre;
	dap->dr_post = dr_post;
	dap_init_jtag(dap);
}

static void q_dap_ir_wr(DAP *dap, u32 ir) {
	if (dap->cached_ir != ir) {
		dap->cached_ir = ir;
		jtag_ir(dap->jtag, 4, ir);
	}
}

// force ir write even if redundant
// used for a timing hack
static void _q_dap_ir_wr(DAP *dap, u32 ir) {
	dap->cached_ir = ir;
	jtag_ir(dap->jtag, 4, ir);
}

static void q_dap_dr_io(DAP *dap, u32 bitcount, u64 wdata, u64 *rdata) {
	if (rdata) {
		*rdata = 0;
	}
	jtag_dr(dap->jtag, bitcount, wdata, rdata);
}

static void q_dap_abort(DAP *dap) {
	jtag_ir(dap->jtag, 4, DAP_IR_ABORT);
	jtag_dr(dap->jtag, 35, 8, NULL);
	dap->cached_ir = 0xFFFFFFFF;
}

static int dap_commit_jtag(DAP *dap) {
	int r = jtag_txn_exec(dap->jtag);
	dap_init_jtag(dap);
	return r;
}

// queue a DPCSW status query, commit jtag txn
static int dap_commit(DAP *dap) {
	u64 a, b;
	q_dap_ir_wr(dap, DAP_IR_DPACC);
	q_dap_dr_io(dap, 35, XPACC_RD(DPACC_CSW), &a);
	q_dap_dr_io(dap, 35, XPACC_RD(DPACC_RDBUFF), &b);
	dap->cached_ir = 0xFFFFFFFF;
	if (dap_commit_jtag(dap)) {
		return -1;
	}
#if 0
	xprintf(XCORE, "DP_CSW %08x(%x) RDBUF %08x(%x)\n",
		((u32) (a >> 3)), ((u32) (a & 7)),
		((u32) (b >> 3)), ((u32) (b & 7)));
#endif
	if (XPACC_STATUS(a) != XPACC_OK) {
		xprintf(XCORE, "dap: invalid txn status\n");
		return -1;
	}
	if (XPACC_STATUS(b) != XPACC_OK) {
		xprintf(XCORE, "dap: cannot read status\n");
		return -1;
	}
	b >>= 3;
	if (b & DPCSW_STICKYORUN) {
		xprintf(XCORE, "dap: overrun\n");
		return -1;
	}
	if (b & DPCSW_STICKYERR) {
		xprintf(XCORE, "dap: error\n");
		return -1;
	}
	return 0;
}

int dap_dp_rd(DAP *dap, u32 addr, u32 *val) {
	u64 u;
	q_dap_ir_wr(dap, DAP_IR_DPACC);
	q_dap_dr_io(dap, 35, XPACC_RD(addr), NULL);
	q_dap_dr_io(dap, 35, XPACC_RD(DPACC_RDBUFF), &u);
	if (dap_commit_jtag(dap)) {
		return -1;
	}
	if (XPACC_STATUS(u) != XPACC_OK) {
		return -1;
	}
	*val = u >> 3;
	return 0;
}

static void q_dap_dp_wr(DAP *dap, u32 addr, u32 val) {
	q_dap_ir_wr(dap, DAP_IR_DPACC);
	q_dap_dr_io(dap, 35, XPACC_WR(addr, val), NULL);
	//q_dap_dr_io(dap, 35, XPACC_RD(DPACC_RDBUFF), &s->u);
}

int dap_dp_wr(DAP *dap, u32 addr, u32 val) {
	q_dap_dp_wr(dap, addr, val);
	return dap_commit_jtag(dap);
}

int dap_ap_rd(DAP *dap, u32 apnum, u32 addr, u32 *val) {
	u64 u;
	q_dap_ir_wr(dap, DAP_IR_DPACC);
	q_dap_dp_wr(dap, DPACC_SELECT, DPSEL_APSEL(apnum) | DPSEL_APBANKSEL(addr));
	q_dap_ir_wr(dap, DAP_IR_APACC);
	q_dap_dr_io(dap, 35, XPACC_RD(addr), NULL);
	q_dap_ir_wr(dap, DAP_IR_DPACC);
	q_dap_dr_io(dap, 35, XPACC_RD(DPACC_RDBUFF), &u);
	// TODO: redundant ir wr
	if (dap_commit(dap)) {
		return -1;
	}
	*val = (u >> 3);
	return 0;
}

void q_dap_ap_wr(DAP *dap, u32 apnum, u32 addr, u32 val) {
	q_dap_ir_wr(dap, DAP_IR_DPACC);
	q_dap_dp_wr(dap, DPACC_SELECT, DPSEL_APSEL(apnum) | DPSEL_APBANKSEL(addr));
	q_dap_ir_wr(dap, DAP_IR_APACC);
	q_dap_dr_io(dap, 35, XPACC_WR(addr, val), NULL);
}

int dap_ap_wr(DAP *dap, u32 apnum, u32 addr, u32 val) {
	q_dap_ap_wr(dap, apnum, addr, val);
	return dap_commit(dap);
}

int dap_mem_wr32(DAP *dap, u32 n, u32 addr, u32 val) {
	if (addr & 3)
		return -1;
	q_dap_ap_wr(dap, n, APACC_CSW,
		0x23000000 | APCSW_DBGSWEN | APCSW_INCR_NONE | APCSW_SIZE32); //XXX
	q_dap_ap_wr(dap, n, APACC_TAR, addr);
	q_dap_ap_wr(dap, n, APACC_DRW, val);
	return dap_commit(dap);
}

int dap_mem_rd32(DAP *dap, u32 n, u32 addr, u32 *val) {
	if (addr & 3)
		return -1;
	q_dap_ap_wr(dap, n, APACC_CSW,
		0x23000000 | APCSW_DBGSWEN | APCSW_INCR_NONE | APCSW_SIZE32); //XXX
	q_dap_ap_wr(dap, n, APACC_TAR, addr);
	if (dap_ap_rd(dap, n, APACC_DRW, val))
		return -1;
	return 0;
}


int dap_attach(DAP *dap) {
	unsigned n;
	u32 x;

	// make sure we abort any ongoing transactions first
	q_dap_abort(dap);

	// attempt to power up and clear errors
	for (n = 0; n < 10; n++) {
		if (dap_dp_wr(dap, DPACC_CSW, CSW_ERRORS | CSW_ENABLES))
			continue;
		if (dap_dp_rd(dap, DPACC_CSW, &x))
			continue;
		if (x & CSW_ERRORS)
			continue;
		if (!(x & DPCSW_CSYSPWRUPACK))
			continue;
		if (!(x & DPCSW_CDBGPWRUPACK))
			continue;
		return 0;
	}
	xprintf(XCORE, "dap: attach failed\n");
	return -1;
}

void dap_clear_errors(DAP *dap) {
	q_dap_dp_wr(dap, DPACC_CSW, CSW_ERRORS | CSW_ENABLES);
}

static int read4xid(DAP *dap, u32 n, u32 addr, u32 *val) {
	u32 a,b,c,d;
	if (dap_mem_rd32(dap, n, addr + 0x00, &a)) return -1;
	if (dap_mem_rd32(dap, n, addr + 0x04, &b)) return -1;
	if (dap_mem_rd32(dap, n, addr + 0x08, &c)) return -1;
	if (dap_mem_rd32(dap, n, addr + 0x0C, &d)) return -1;
	*val = (a & 0xFF) | ((b & 0xFF) << 8) | ((c & 0xFF) << 16) | ((d & 0xFF) << 24);
	return 0;
}

static int readinfo(DAP *dap, u32 n, u32 base, u32 *cid, u32 *pid0, u32 *pid1) {
	if (read4xid(dap, n, base + 0xFF0, cid)) return -1;
	if (read4xid(dap, n, base + 0xFE0, pid0)) return -1;
	if (read4xid(dap, n, base + 0xFD0, pid1)) return -1;
	return 0;
}

static void dumptable(DAP *dap, u32 n, u32 base) {
	u32 cid, pid0, pid1, memtype;
	u32 x, addr;
	int i;

	xprintf(XCORE, "TABLE   @%08x ", base);
	if (readinfo(dap, n, base, &cid, &pid0, &pid1)) {
		xprintf(XCORE, "<error reading cid & pid>\n");
		return;
	}
	if (dap_mem_rd32(dap, n, base + 0xFCC, &memtype)) {
		xprintf(XCORE, "<error reading memtype>\n");
		return;
	}
	xprintf(XCORE, "CID %08x  PID %08x %08x  %dKB%s\n", cid, pid1, pid0,
		4 * (1 + ((pid1 & 0xF0) >> 4)),
		(memtype & 1) ? "  SYSMEM": "");
	for (i = 0; i < 128; i++) {
		if (dap_mem_rd32(dap, n, base + i * 4, &x)) break;
		if (x == 0) break;
		if ((x & 3) != 3) continue;
		addr = base + (x & 0xFFFFF000);
		if (readinfo(dap, n, addr, &cid, &pid0, &pid1)) {
			xprintf(XCORE, "    <error reading cid & pid>\n");
			continue;
		}
		xprintf(XCORE, "    %02d: @%08x CID %08x  PID %08x %08x  %dKB\n",
			i, addr, cid, pid1, pid0,
			4 * (1 + ((pid1 & 0xF0) >> 4)));
		if (((cid >> 12) & 0xF) == 1) {
			dumptable(dap, n, addr);
		}
	}
}

int dap_probe(DAP *dap) {
	unsigned n;
	u32 x, y;

	for (n = 0; n < 256; n++) {
		if (dap_ap_rd(dap, n, APACC_IDR, &x))
			break;
		if (x == 0)
			break;
		y = 0;
		dap_ap_rd(dap, n, APACC_BASE, &y);
		xprintf(XCORE, "AP%d ID=%08x BASE=%08x\n", n, x, y);
		if (y && (y != 0xFFFFFFFF)) {
			dumptable(dap, n, y & 0xFFFFF000);
		}
		if (dap_ap_rd(dap, n, APACC_CSW, &x) == 0)
			xprintf(XCORE, "AP%d CSW=%08x\n", n, x);
	}
	return 0;
}

static int jtag_error = 0;

#include "ti-icepick.h"

#define IDCODE_ARM_M3		0x4ba00477
#define IPCODE_TI_ICEPICK	0x41611cc0
#define IDCODE_CC1310		0x2b9be02f
#define IDCODE_CC2650		0x8b99a02f

int connect_ti(void) {
	u64 x0, x1;
	int retry = 5;
	jtag_txn t;

	while (retry > 0) {
		jtag_txn_init(&t);
		jtag_any_to_rti(&t);

		// Enable 4-wire JTAG
		jtag_ir(&t, 6, 0x3F);
		jtag_cjtag_open(&t);
		jtag_cjtag_cmd(&t, 2, 9);
		jtag_ir(&t, 6, 0x3F);

		// sit in IDLE for a bit (not sure if useful)
		jtag_txn_append(&t, 64, 0, 0, NULL);

		// Read IDCODE and IPCODE
		jtag_ir(&t, 6, IP_IR_IDCODE);
		jtag_dr(&t, 32, 0x00000000, &x0);
		jtag_ir(&t, 6, IP_IR_IPCODE);
		jtag_dr(&t, 32, 0x00000000, &x1);
		jtag_txn_exec(&t);

		if (x1 == IPCODE_TI_ICEPICK) {
			if (x0 == IDCODE_CC1310) {
				break;
			}
			if (x0 == IDCODE_CC2650) {
				break;
			}
			xprintf(XCORE, "wat!\n");
		}
		retry--;
	}
	if (retry == 0) {
		//xprintf(XCORE, "cannot connect (%08x %08x)\n", (u32)x0, (u32)x1);
		return -1;
	}
	xprintf(XSWD, "attach: IDCODE %08x %08x\n", (u32)x0, (u32)x1);

	jtag_txn_init(&t);
	t.state = JTAG_IDLE;

	// enable router access
	jtag_ir(&t, 6, IP_IR_CONNECT);
	jtag_dr(&t, 8, IP_CONNECT_WR_KEY, NULL);

	// add ARM DAP to the scan chain
	jtag_ir(&t, 6, IP_IR_ROUTER);
	jtag_dr_p(&t, 32, IP_RTR_WR | IP_RTR_BLK(IP_BLK_DEBUG_TLCB) |
		IP_RTR_REG(IP_DBG_TAP0) |
		IP_RTR_VAL(IP_DBG_SELECT_TAP | IP_DBG_INHIBIT_SLEEP | IP_DBG_FORCE_ACTIVE), NULL);

	// commit router changes by going to IDLE state
	jtag_txn_move(&t, JTAG_IDLE);

	// idle for a few clocks to let the new TAP path settle
	jtag_txn_append(&t, 8, 0, 0, NULL);
	jtag_txn_exec(&t);

#if 0
	jtag_txn_init(&t);
	t.state = JTAG_IDLE;
	t.ir_pre = 4;
	t.dr_pre = 1;
	jtag_ir(&t, 6, IP_IR_ROUTER);
	jtag_dr(&t, 32, IP_RTR_WR | IP_RTR_BLK(IP_BLK_CONTROL) | IP_RTR_REG(IP_CTL_CONTROL) |
		IP_RTR_VAL(IP_CTL_SYSTEM_RESET), NULL);
	jtag_txn_exec(&t);
#endif

	// we should now be stable, try to read DAP IDCODE
	jtag_txn_init(&t);
	t.state = JTAG_IDLE;
	t.ir_post = 6;
	t.dr_post = 1;
	jtag_ir(&t, 4, 0xE); // IDCODE
	jtag_dr(&t, 32, 0, &x0);
	jtag_txn_exec(&t);

	if (x0 != IDCODE_ARM_M3) {
		xprintf(XDATA, "cannot find DAP (%08x)\n", (u32)x0);
		return -1;
	}

	return 0;
}

int _attach(void) {
	jtag_txn t;
	DAP dap;
	jtag_error = -1;
	if (connect_ti()) {
		return -1;
	}
	dap_init(&dap, &t, 0, 6, 0, 1);
	if (dap_attach(&dap)) {
		return -1;
	}
	xprintf(XSWD, "attach: JTAG DAP ok\n");
	jtag_error = 0;
	return 0;
}

static int _mem_rd_32(u32 addr, u32 *val) {
	jtag_txn t;
	DAP dap;
	if ((addr & 3) || jtag_error) {
		return -1;
	}
	dap_init(&dap, &t, 0, 6, 0, 1);
	jtag_error = dap_mem_rd32(&dap, 0, addr, val);
	return jtag_error;
}

static int _mem_wr_32(u32 addr, u32 val) {
	jtag_txn t;
	DAP dap;
	if ((addr & 3) || jtag_error) {
		return -1;
	}
	dap_init(&dap, &t, 0, 6, 0, 1);
	jtag_error = dap_mem_wr32(&dap, 0, addr, val);
	return jtag_error;
}

static int _mem_rd_32_c(u32 addr, u32 *data, int count) {
	jtag_txn t;
	DAP dap;
	if ((addr & 3) || jtag_error) {
		return -1;
	}
	dap_init(&dap, &t, 0, 6, 0, 1);
	while (count > 0) {
		if (dap_mem_rd32(&dap, 0, addr, data)) {
			jtag_error = -1;
			return -1;
		}
		addr += 4;
		data++;
		count--;
	}
	return 0;
}

static int _mem_wr_32_c(u32 addr, u32 *data, int count) {
	jtag_txn t;
	DAP dap;
	if ((addr & 3) || jtag_error) {
		return -1;
	}
	dap_init(&dap, &t, 0, 6, 0, 1);
	while (count > 0) {
		if (dap_mem_wr32(&dap, 0, addr, *data)) {
			jtag_error = -1;
			return -1;
		}
		addr += 4;
		data++;
		count--;
	}
	return 0;
}

static int _clear_error(void) {
	if (jtag_error) {
		//XXX this is a lighter weight operation in SWDP
		_attach();
	}
	return jtag_error;
}

static int _error(void) {
	return jtag_error;
}

debug_transport JTAG_TRANSPORT = {
	.attach = _attach,
	.error = _error,
	.clear_error = _clear_error,
	.mem_rd_32 = _mem_rd_32,
	.mem_wr_32 = _mem_wr_32,
	.mem_rd_32_c = _mem_rd_32_c,
	.mem_wr_32_c = _mem_wr_32_c,
};
