/* jtag transport
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

#include <string.h>
#include <stdlib.h>

#include <fw/types.h>

#include "debugger.h"
#include "rswdp.h"
#include "jtag.h"

static const char *JSTATE[17] = {
        "RESET", "IDLE", "DRSELECT", "DRCAPTURE",
        "DRSHIFT", "DREXIT1", "DRPAUSE", "DREXIT2",
        "DRUPDATE", "IRSELECT", "IRCAPTURE", "IRSHIFT",
        "IREXIT1", "IRPAUSE", "IREXIT1", "IRUPDATE",
	"UNKNOWN"
};

static u64 ONES = 0xFFFFFFFFFFFFFFFFUL;

void jtag_txn_init(jtag_txn *tx) {
	memset(tx, 0, sizeof(jtag_txn));
	tx->state = JTAG_UNKNOWN;
}

int jtag_txn_exec(jtag_txn *t) {
	unsigned n, off = 0;
	int r;
	if (t->status) {
		return t->status;
	}
	//xprintf(XCORE, "jtag exec %d bits\n", t->txc);
	r = jtag_io(t->txc, t->tms, t->tdi, t->tdo);
	for (n = 0; n < t->rxc; n++) {
		unsigned count = t->bits[n];
		if (t->ptr[n]) {
			unsigned bit = 0;
			u64 x = 0;
			while (count > 0) {
				x |= ((u64) ((t->tdo[off >> 5] >> (off & 31)) & 1)) << bit;
				off++;
				bit++;
				count--;
			}
			*t->ptr[n] = x;
		} else {
			off += count;
		}
	}
	return r;
}

void jtag_txn_append(jtag_txn *t, unsigned count, u64 tms, u64 tdi, u64 *tdo) {
	unsigned txc = t->txc;

	if (txc == JTAG_MAX_RESULTS) {
		xprintf(XCORE, "jtag append txn overflow\n");
		t->status = -1;
		return;
	}
	if ((count > 64) || ((JTAG_MAX_BITS - t->bitcount) < count)) {
		xprintf(XCORE, "jtag append bits overflow\n");
		t->status = -1;
		return;
	}
	t->bitcount += count;

//	xprintf(XCORE, "jtag append %2d bits %016lx %016lx\n", count, tms, tdi);

	t->ptr[t->rxc] = tdo;
	t->bits[t->rxc] = count;
	t->rxc++;

	while (count > 0) {
		t->tms[txc >> 5] |= (tms & 1) << (txc & 31);
		t->tdi[txc >> 5] |= (tdi & 1) << (txc & 31);
		tms >>= 1;
		tdi >>= 1;
		count--;
		txc++;
	}
	t->txc = txc;
}

void jtag_txn_move(jtag_txn *t, unsigned dst) {
	if (t->state == dst) {
		// nothing to do
		return;
	}
	switch (t->state) {
	case JTAG_IDLE:
		if (dst == JTAG_IRSHIFT) {
			// Idle -> SelDR -> SelIR -> CapIR -> ShiftIR
			//      1        1        0        0
			jtag_txn_append(t, 4, 0b0011, 0b1111, NULL);
		} else if (dst == JTAG_DRSHIFT) {
			// Idle -> SelDR -> CapDR -> ShiftDR
			//      1        0        0
			jtag_txn_append(t, 3, 0b001, 0b111, NULL);
		} else {
			goto oops;
		}
		break;
	case JTAG_DRPAUSE:
	case JTAG_IRPAUSE:
		if (dst == JTAG_IRSHIFT) {
			// PauseDR -> Exit2DR -> UpDR -> SelDR -> SelIR -> CapIR -> ShiftIR
			// PauseIR -> Exit2IR -> UpIR -> SelDR -> SelIR -> CapIR -> ShiftIR
			//         1          1       1        1        0        0
			jtag_txn_append(t, 6, 0b001111, 0b111111, NULL);
		} else if (dst == JTAG_DRSHIFT) {
			// PauseDR -> Exit2DR -> UpDR -> SelDR -> CapDR -> ShiftDR
			// PauseIR -> Exit2IR -> UpIR -> SelDR -> CapDR -> ShiftDR
			//         1          1       1        0        0
			jtag_txn_append(t, 5, 0b00111, 0b11111, NULL);
		} else if (dst == JTAG_IDLE) {
			// PauseDR -> Exit2DR -> UpDR -> Idle
			// PauseIR -> Exit2IR -> UpIR -> Idle
			//         1          1       0
			jtag_txn_append(t, 3, 0b011, 0b111, NULL);
		} else {
			goto oops;
		}
		break;
	case JTAG_DREXIT1:
	case JTAG_IREXIT1:
		if (dst == JTAG_IRSHIFT) {
			// Exit1DR -> UpDR -> SelDR -> SelIR -> CapIR -> ShiftIR
			// Exit1IR -> UpIR -> SelDR -> SelIR -> CapIR -> ShiftIR
			//         1       1        1        0        0
			jtag_txn_append(t, 5, 0b00111, 0b11111, NULL);
		} else if (dst == JTAG_DRSHIFT) {
			// Exit1DR -> UpDR -> SelDR -> CapDR -> ShiftDR
			// Exit1IR -> UpIR -> SelDR -> CapDR -> ShiftDR
			//         1       1        0        0
			jtag_txn_append(t, 4, 0b0011, 0b1111, NULL);
		} else if (dst == JTAG_IDLE) {
			// Exit1DR -> UpDR -> Idle
			// Exit1IR -> UpIR -> Idle
			//         1       0
			jtag_txn_append(t, 2, 0b01, 0b11, NULL);
		} else if (dst == JTAG_DRPAUSE) {
			// Exit1DR -> PauseDR
			// Exit1IR -> PauseIR
			//         0
			jtag_txn_append(t, 1, 0b0, 0b1, NULL);
		} else {
			goto oops;
		}
		break;
	default:
oops:
		xprintf(XCORE, "jtag move from %s to %s unsupported\n",
			JSTATE[t->state], JSTATE[dst]);
		t->status = -1;
		t->state = JTAG_UNKNOWN;
		return;
	}
	t->state = dst;
}

void jtag_cjtag_open(jtag_txn *t) {
	jtag_txn_move(t, JTAG_IDLE);
	// Idle -> SelDR -> CapDR -> Exit1DR -> UpDR             ZBS#1
	//      1        0        1          1
	// UpDR -> SelDR -> CapDR -> Exit1DR -> UpDR             ZBS#2
        //      1        0        1          1
	// UpDR -> SelDR -> CapDR -> ShiftDR -> Exit1DR -> UpDR -> Idle   One Bit Scan
	//      1        0        0          1          1       0
	jtag_txn_append(t, 14, 0b01100111011101, 0, NULL);
} 

void jtag_cjtag_cmd(jtag_txn *t, unsigned cp0, unsigned cp1) {
	jtag_txn_move(t, JTAG_IDLE);
	if ((cp0 > 15) || (cp1 > 15)) {
		xprintf(XCORE, "jtag invalid cjtag parameters %d %d\n", cp0, cp1);
		t->status = -1;
		return;
	}
	if (cp0 == 0) {
		// Idle -> SelDR -> CapDR -> Exit1DR -> UpDR -> Idle
		//      1        0        1          1       0
		jtag_txn_append(t, 5, 0b01101, 0, NULL);
	} else {
		// Idle -> SelDR -> CapDR -> ShiftDR (xN) -> Exit1DR -> UpDR -> Idle
		//      1        0        0               1          1       0
		jtag_txn_append(t, 5 + cp0, ((0b011 << cp0) << 2) | 0b01, 0, NULL);
	}
	if (cp1 == 0) {
		// Idle -> SelDR -> CapDR -> Exit1DR -> UpDR -> Idle
		//      1        0        1          1       0
		jtag_txn_append(t, 5, 0b01101, 0, NULL);
	} else {
		// Idle -> SelDR -> CapDR -> ShiftDR (xN) -> Exit1DR -> UpDR -> Idle
		//      1        0        0               1          1       0
		jtag_txn_append(t, 5 + cp1, ((0b011 << cp1) << 2) | 0b01, 0, NULL);
	}
}

void jtag_any_to_rti(jtag_txn *t) {
	// 5x TMS=1 is sufficient, but throw a couple more in for luck
	jtag_txn_append(t, 9, 0xFF, 0x1FF, NULL);
	t->state = JTAG_IDLE;
}

static void _jtag_shiftir(jtag_txn *t, unsigned count, u64 tx, u64 *rx) {
	// all bits but last, tms=0 (stay in ShiftIR), last tms=1 (goto Exit1IR)
	if (t->state != JTAG_IRSHIFT) {
		xprintf(XCORE, "jtag invalid state (%s) in shiftir\n", JSTATE[t->state]);
		t->status = -1;
	}
	if (t->ir_pre) {
		jtag_txn_append(t, t->ir_pre, 0, ONES, NULL);
	}
	if (t->ir_post) {
		u64 tms = 1UL << (t->ir_post - 1);
		jtag_txn_append(t, count, 0, tx, rx);
		jtag_txn_append(t, t->ir_post, tms, ONES, NULL);
	} else {
		u64 tms = 1UL << (count - 1);
		jtag_txn_append(t, count, tms, tx, rx);
	}
	t->state = JTAG_IREXIT1;
}

static void _jtag_shiftdr(jtag_txn *t, unsigned count, u64 tx, u64 *rx) {
	if (t->state != JTAG_DRSHIFT) {
		xprintf(XCORE, "jtag invalid state (%s) in shiftir\n", JSTATE[t->state]);
		t->status = -1;
	}
	// all bits but last, tms=0 (stay in ShiftDR), last tms=1 (goto Exit1DR)
	if (t->dr_pre) {
		jtag_txn_append(t, t->dr_pre, 0, ONES, NULL);
	}
	if (t->dr_post) {
		u64 tms = 1UL << (t->dr_post - 1);
		jtag_txn_append(t, count, 0, tx, rx);
		jtag_txn_append(t, t->dr_post, tms, ONES, NULL);
	} else {
		u64 tms = 1UL << (count - 1);
		jtag_txn_append(t, count, tms, tx, rx);
	}
	t->state = JTAG_DREXIT1;
}

void jtag_ir(jtag_txn *t, unsigned count, u64 ir) {
	jtag_txn_move(t, JTAG_IRSHIFT);
	_jtag_shiftir(t, count, ir, NULL);
	jtag_txn_move(t, JTAG_IDLE);
}

void jtag_dr(jtag_txn *t, unsigned count, u64 dr, u64 *out) {
	jtag_txn_move(t, JTAG_DRSHIFT);
	_jtag_shiftdr(t, count, dr, out);
	jtag_txn_move(t, JTAG_IDLE);
}

void jtag_ir_p(jtag_txn *t, unsigned count, u64 ir) {
	jtag_txn_move(t, JTAG_IRSHIFT);
	_jtag_shiftir(t, count, ir, NULL);
	jtag_txn_move(t, JTAG_IRPAUSE);
}

void jtag_dr_p(jtag_txn *t, unsigned count, u64 dr, u64 *out) {
	jtag_txn_move(t, JTAG_DRSHIFT);
	_jtag_shiftdr(t, count, dr, out);
	jtag_txn_move(t, JTAG_DRPAUSE);
}


