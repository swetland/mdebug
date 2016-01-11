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

#ifndef _JTAG_H_
#define _JTAG_H_

#define JTAG_RESET      0
#define JTAG_IDLE       1
#define JTAG_DRSELECT   2
#define JTAG_DRCAPTURE  3
#define JTAG_DRSHIFT    4
#define JTAG_DREXIT1    5
#define JTAG_DRPAUSE    6
#define JTAG_DREXIT2    7
#define JTAG_DRUPDATE   8
#define JTAG_IRSELECT   9
#define JTAG_IRCAPTURE  10
#define JTAG_IRSHIFT    11
#define JTAG_IREXIT1    12
#define JTAG_IRPAUSE    13
#define JTAG_IREXIT2    14
#define JTAG_IRUPDATE   15
#define JTAG_UNKNOWN	16


#define JTAG_MAX_WORDS		256
#define JTAG_MAX_BITS		((JTAG_MAX_WORDS) * 4)
#define JTAG_MAX_RESULTS	256

typedef struct jtag_txn {
	u32 tms[JTAG_MAX_WORDS];
	u32 tdi[JTAG_MAX_WORDS];
	u32 tdo[JTAG_MAX_WORDS];
	u8 bits[JTAG_MAX_RESULTS];
	u64 *ptr[JTAG_MAX_RESULTS];
	unsigned txc;
	unsigned rxc;
	unsigned bitcount;
	int status;
	u32 ir_pre;
	u32 ir_post;
	u32 dr_pre;
	u32 dr_post;
	unsigned state;
} jtag_txn;


void jtag_txn_init(jtag_txn *tx);
int jtag_txn_exec(jtag_txn *t);

void jtag_txn_move(jtag_txn *t, unsigned dst);

// clock out 5+ TMS 1s and get to RESET, then IDLE from anywhere
void jtag_any_to_rti(jtag_txn *t);

// scan into IR or DR and end in IDLE state
void jtag_ir(jtag_txn *t, unsigned count, u64 ir);
void jtag_dr(jtag_txn *t, unsigned count, u64 dr, u64 *out);

// scan into IR or DR but end in PauseIR or PauseDR state, not IDLE
void jtag_ir_p(jtag_txn *t, unsigned count, u64 ir);
void jtag_dr_p(jtag_txn *t, unsigned count, u64 dr, u64 *out);

void jtag_cjtag_open(jtag_txn *t);
void jtag_cjtag_cmd(jtag_txn *t, unsigned cp0, unsigned cp1);

// clock raw TMS/TDI bit streams, does not check or update t->state
void jtag_txn_append(jtag_txn *t, unsigned count, u64 tms, u64 tdi, u64 *tdo);


#endif

