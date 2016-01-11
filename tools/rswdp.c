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

#include <pthread.h>

#include "usb.h"

#include <fw/types.h>
#include <protocol/rswdp.h>
#include "rswdp.h"
#include "arm-v7m.h"

#include "debugger.h"

int swd_verbose = 0;

static pthread_mutex_t swd_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t swd_event = PTHREAD_COND_INITIALIZER;
static pthread_t swd_thread;

// these are all protected by swd_lock
static u16 sequence = 1;
static int swd_online = 0;
static usb_handle *usb = NULL;

static unsigned swd_txn_id = 0;
static void *swd_txn_data = NULL;
static int swd_txn_status = 0;

#define TXN_STATUS_WAIT		-2
#define TXN_STATUS_FAIL		-1

// swd_thread is responsible for setting swd_online to 1 once
// the USB connection is active.
//
// In the event of a usb connection error, swd_thread sets
// swd_online to -1, and the next swd io attempt must acknowledge
// this by zeroing the usb handle and setting swd_online to 0
// at which point the swd_thread may attempt to reconnect.

#define MAXWORDS (8192/4)
static unsigned swd_maxwords = 512;
static unsigned swd_version = 0x0001;

static int swd_error = 0;

static int _swdp_error(void) {
	return swd_error;
}

struct txn {
	/* words to transmit */
	u32 tx[MAXWORDS];
	/* pointers to words for reply data */
	u32 *rx[MAXWORDS];

	/* words to send and receive */
	unsigned txc;
	unsigned rxc;

	/* cached state */
	u32 cache_apaddr;
	u32 cache_ahbtar;

	unsigned magic;
};

void process_swo_data(void *data, unsigned count);
void transmit_swo_data(void *data, unsigned count);

static void process_async(u32 *data, unsigned count) {
	unsigned msg, n;
	u32 tmp;
	while (count-- > 0) {
		msg = *data++;
		switch (RSWD_MSG_CMD(msg)) {
		case CMD_NULL:
			continue;
		case CMD_DEBUG_PRINT:
			n = RSWD_MSG_ARG(msg);
			if (n > count)
				return;
			tmp = data[n];
			data[n] = 0;
			xprintf(XSWD, "%s",(char*) data);
			data[n] = tmp;
			data += n;
			count -= n;
			break;
		case CMD_SWO_DATA:
			n = RSWD_MSG_ARG(msg);
			if (swd_version < RSWD_VERSION_1_1) {
				// arg is wordcount
				tmp = n;
				n *= 4;
			} else {
				// arg is bytecount
				tmp = (n + 3) / 4;
			}
			if (tmp > count)
				return;
			process_swo_data(data, n);
			transmit_swo_data(data, n);
			data += tmp;
			count -= tmp;
		default:
			return;
		}
	}
}

static void process_query(u32 *data, unsigned count) {
	unsigned n;
	const char *board = "unknown";
	const char *build = "unknown";
	unsigned version = 0x0005;
	unsigned maxdata = 2048;

	while (count-- > 0) {
		unsigned msg = *data++;
		switch (RSWD_MSG_CMD(msg)) {
		case CMD_NULL:
			break;
		case CMD_BUILD_STR:
			n = RSWD_MSG_ARG(msg);
			if (n > count) goto done;
			build = (void*) data;
			data += n;
			break;
		case CMD_BOARD_STR:
			n = RSWD_MSG_ARG(msg);
			if (n > count) goto done;
			board = (void*) data;
			data += n;
			break;
		case CMD_VERSION:
			version = RSWD_MSG_ARG(msg);
			break;
		case CMD_RX_MAXDATA:
			maxdata = RSWD_MSG_ARG(msg);
			break;
		default:
			goto done;
		}
	}
done:
	if (maxdata > (MAXWORDS * 4)) {
		maxdata = MAXWORDS * 4;
	}
	xprintf(XSWD, "usb: board id: %s\n", board);
	xprintf(XSWD, "usb: build id: %s\n", build);
	xprintf(XSWD, "usb: protocol: %d.%d\n", version >> 8, version & 0xff);
	xprintf(XSWD, "usb: max data: %d byte rx buffer\n", maxdata);

	swd_version = version;
	swd_maxwords = maxdata / 4;
}

const char *swd_err_str(unsigned op) {
	switch (op) {
	case ERR_NONE: return "NONE";
	case ERR_INTERNAL: return "INTERNAL";
	case ERR_TIMEOUT: return "TIMEOUT";
	case ERR_IO: return "IO";
	case ERR_PARITY: return "PARITY";
	default: return "UNKNOWN";
	}
}

static int process_reply(struct txn *t, u32 *data, int count) {
	unsigned msg, op, n, rxp, rxc;

	rxc = t->rxc;
	rxp = 0;

	while (count-- > 0) {
		msg = *data++;
		op = RSWD_MSG_OP(msg);
		n = RSWD_MSG_ARG(msg);

		//fprintf(stderr,"[ %02x %02x %04x ]\n",RSWD_MSG_CMD(msg), op, n);
		switch (RSWD_MSG_CMD(msg)) {
		case CMD_NULL:
			continue;
		case CMD_SWD_DATA:
			if (n > rxc) {
				xprintf(XSWD, "reply overrun (%d > %d)\n", n, rxc);
				return -1;
			}
			while (n-- > 0) {
				//printf("data %08x -> %p\n", data[0], t->rx[rxp]);
				*(t->rx[rxp++]) = *data++;
				rxc--;
			}
			continue;
		case CMD_JTAG_DATA:
			//xprintf(XSWD, "JTAG DATA %d bits\n", n);
			if (((n+31)/32) > rxc) {
				xprintf(XSWD, "reply overrun (%d bits > %d words)\n", n, rxc);
				return -1;
			}
			n = (n + 31) / 32;
			while (n > 0) {
				//xprintf(XSWD, "JTAG %08x\n", *data);
				*(t->rx[rxp++]) = *data++;
				rxc--;
				n--;
			}
			continue;
		case CMD_STATUS:
			if (op) {
				if (swd_verbose) {
					xprintf(XSWD, "SWD ERROR: %s\n", swd_err_str(op));
				}
				swd_error = -op;
				return -op;
			} else {
				return 0;
			}
		case CMD_CLOCK_KHZ:
			xprintf(XSWD,"mdebug: SWD clock: %d KHz\n", n);
			continue;
		default:
			xprintf(XSWD,"unknown command 0x%02x\n", RSWD_MSG_CMD(msg));
			return -1;
		}
	}
	return 0;
}

static int q_exec(struct txn *t) {
	unsigned data[MAXWORDS];
	unsigned seq;
	int r;
	u32 id;

	if (t->magic != 0x12345678) {
		fprintf(stderr,"FATAL: bogus txn magic\n");
		exit(1);
	}
	t->magic = 0;

	/* If we are a multiple of 64, and not exactly 4K,
	 * add padding to ensure the target can detect the end of txn
	 */
	if (((t->txc % 16) == 0) && (t->txc != swd_maxwords))
		t->tx[t->txc++] = RSWD_MSG(CMD_NULL, 0, 0);

	pthread_mutex_lock(&swd_lock);
 	seq = sequence++;
	id = RSWD_TXN_START(seq);
	t->tx[0] = id;

	if (swd_online != 1) {
		if (swd_online == -1) {
			// ack disconnect
			usb_close(usb);
			usb = NULL;
			swd_online = 0;
			pthread_cond_broadcast(&swd_event);
		}
		r = -1;
	} else {
		r = usb_write(usb, t->tx, t->txc * sizeof(u32));
		if (r == (t->txc * sizeof(u32))) {
			swd_txn_id = id;
			swd_txn_data = data;
			swd_txn_status = TXN_STATUS_WAIT;
			do {
				pthread_cond_wait(&swd_event, &swd_lock);
			} while (swd_txn_status == TXN_STATUS_WAIT);
			if (swd_txn_status == TXN_STATUS_FAIL) {
				r = -1;
			} else {
				r = swd_txn_status;
			}
			swd_txn_data = NULL;
		} else {
			r = -1;
		}
	}
	pthread_mutex_unlock(&swd_lock);
	if (r > 0) {
		return process_reply(t, data + 1, (r / 4) - 1);
	} else {
		return -1;
	}
}

static void *swd_reader(void *arg) {
	uint32_t data[MAXWORDS];
	unsigned query_id;
	int r;
	int once = 1;
restart:
	for (;;) {
		if ((usb = usb_open(0x1209, 0x5038, 0))) break;
		if ((usb = usb_open(0x18d1, 0xdb03, 0))) break;
		if ((usb = usb_open(0x18d1, 0xdb04, 0))) break;
		if (once) {
			xprintf(XSWD, "usb: waiting for debugger device\n");
			once = 0;
		}
		usleep(250000);
	}
	once = 0;
	xprintf(XSWD, "usb: debugger connected\n");

	pthread_mutex_lock(&swd_lock);

	// send a version query to find out about the firmware
	// old m3debug fw will just report failure
 	query_id = sequence++;
	query_id = RSWD_TXN_START(query_id);
	data[0] = query_id;
	data[1] = RSWD_MSG(CMD_VERSION, 0, RSWD_VERSION);
	usb_write(usb, data, 8);
	for (;;) {
		pthread_mutex_unlock(&swd_lock);
		r = usb_read_forever(usb, data, MAXWORDS * 4);
		pthread_mutex_lock(&swd_lock);
		if (r < 0) {
			xprintf(XSWD, "usb: debugger disconnected\n");
			swd_online = -1;
			swd_txn_status = TXN_STATUS_FAIL;
			pthread_cond_broadcast(&swd_event);
			break;
		}
		if ((r < 4) || (r & 3)) {
			xprintf(XSWD, "usb: discard packet (%d)\n", r);
			continue;
		}
		if (query_id && (data[0] == query_id)) {
			query_id = 0;
			process_query(data + 1, (r / 4) - 1);
			swd_online = 1;
		} else if (data[0] == RSWD_TXN_ASYNC) {
			pthread_mutex_unlock(&swd_lock);
			process_async(data + 1, (r / 4) - 1);
			pthread_mutex_lock(&swd_lock);
		} else if ((swd_txn_status == TXN_STATUS_WAIT) &&
			(data[0] == swd_txn_id)) {
			swd_txn_status = r;
			memcpy(swd_txn_data, data, r);
			pthread_cond_broadcast(&swd_event);
		} else {
			xprintf(XSWD, "usb: rx: unexpected txn %08x (%d)\n", data[0], r);
		}
	}
	// wait for a reader to ack the shutdown (and close usb)
	while (swd_online == -1) {
		pthread_cond_wait(&swd_event, &swd_lock);
	}
	pthread_mutex_unlock(&swd_lock);
	usleep(250000);
	goto restart;
	return NULL;
}

static void q_check(struct txn *t, int n) {
	if ((t->txc + n) >= MAXWORDS) {
		fprintf(stderr,"FATAL: txn buffer overflow\n");
		exit(1);
	}
}

static void q_init(struct txn *t) {
	t->magic = 0x12345678;
	t->txc = 1;
	t->rxc = 0;
	t->cache_apaddr = 0xffffffff;
	t->cache_ahbtar = 0xffffffff;
}

#define SWD_WR(a,n) RSWD_MSG(CMD_SWD_WRITE, OP_WR | (a), (n))
#define SWD_RD(a,n) RSWD_MSG(CMD_SWD_READ, OP_RD | (a), (n))
#define SWD_RX(a,n) RSWD_MSG(CMD_SWD_DISCARD, OP_RD | (a), (n))

static void q_ap_select(struct txn *t, u32 addr) {
	addr &= 0xF0;
	if (t->cache_apaddr != addr) {
		t->tx[t->txc++] = SWD_WR(DP_SELECT, 1);
		t->tx[t->txc++] = addr;
		t->cache_apaddr = addr;
	}
}

static void q_ap_write(struct txn *t, u32 addr, u32 value) {
	q_check(t, 3);
	q_ap_select(t, addr);
	t->tx[t->txc++] = SWD_WR(OP_AP | (addr & 0xC), 1);
	t->tx[t->txc++] = value;
}

static void q_ap_read(struct txn *t, u32 addr, u32 *value) {
	q_check(t, 4);
	q_ap_select(t, addr);
	t->tx[t->txc++] = SWD_RX(OP_AP | (addr & 0xC), 1);
	t->tx[t->txc++] = SWD_RD(DP_BUFFER, 1);
	t->rx[t->rxc++] = value;
}

static void q_ahb_write(struct txn *t, u32 addr, u32 value) {
	if (t->cache_ahbtar != addr) {
		q_ap_write(t, AHB_TAR, addr);
		t->cache_ahbtar = addr;
	}
	q_ap_write(t, AHB_DRW, value);
}

static void q_ahb_read(struct txn *t, u32 addr, u32 *value) {
	if (t->cache_ahbtar != addr) {
		q_ap_write(t, AHB_TAR, addr);
		t->cache_ahbtar = addr;
	}
	q_ap_read(t, AHB_DRW, value);
}

int swdp_ap_write(u32 addr, u32 value) {
	struct txn t;
	q_init(&t);
	q_ap_write(&t, addr, value);
	return q_exec(&t);
}

int swdp_ap_read(u32 addr, u32 *value) {
	struct txn t;
	q_init(&t);
	q_ap_read(&t, addr, value);
	return q_exec(&t);
}

static int _swdp_ahb_read(u32 addr, u32 *value) {
	struct txn t;
	q_init(&t);
	q_ahb_read(&t, addr, value);
	return q_exec(&t);
}

static int _swdp_ahb_write(u32 addr, u32 value) {
	struct txn t;
	q_init(&t);
	q_ahb_write(&t, addr, value);
	return q_exec(&t);
}

#if 0
/* simpler but far less optimal. keeping against needing to debug */
int swdp_ahb_read32(u32 addr, u32 *out, int count) {
	struct txn t;
	while (count > 0) {
		int xfer = (count > 128) ? 128: count;
		count -= xfer;
		q_init(&t);
		while (xfer-- > 0) {
			q_ahb_read(&t, addr, out++);
			addr += 4;
		}
		if (q_exec(&t))
			return -1;
	}
	return 0;
}

int swdp_ahb_write32(u32 addr, u32 *in, int count) {
	struct txn t;
	while (count > 0) {
		int xfer = (count > 128) ? 128: count;
		count -= xfer;
		q_init(&t);
		while (xfer-- > 0) {
			q_ahb_write(&t, addr, *in++);
			addr += 4;
		}
		if (q_exec(&t))
			return -1;
	}
	return 0;
}
#else
#define MAXDATAWORDS (swd_maxwords - 16)
/* 10 txns overhead per 128 read txns - 126KB/s on 72MHz STM32F
 * 8 txns overhead per 128 write txns - 99KB/s on 72MHz STM32F
 */
static int _swdp_ahb_read32(u32 addr, u32 *out, int count) {
	struct txn t;

	while (count > 0) {
		int xfer;

		/* auto-inc wraps at 4K page boundaries -- limit max
		 * transfer so we won't cross a page boundary
		 */
		xfer = (0x1000 - (addr & 0xFFF)) / 4;
		if (xfer > count)
			xfer = count;
		if (xfer > MAXDATAWORDS)
			xfer = MAXDATAWORDS;

		count -= xfer;
		q_init(&t);

		/* setup before initial txn */
		q_ap_write(&t, AHB_CSW,
			AHB_CSW_MDEBUG | AHB_CSW_PRIV | AHB_CSW_INC_SINGLE |
			AHB_CSW_DBG_EN | AHB_CSW_32BIT);

		/* initial address */
		q_ap_write(&t, AHB_TAR, addr);
		addr += xfer * 4;

		/* kick off first read, ignore result, as the
		 * real result will show up during the *next* read
		 */
		t.tx[t.txc++] = SWD_RX(OP_AP | (AHB_DRW & 0xC), 1);
		t.tx[t.txc++] = SWD_RD(OP_AP | (AHB_DRW & 0xC), xfer -1);
		while (xfer-- > 1)
			t.rx[t.rxc++] = out++;
		t.tx[t.txc++] = SWD_RD(DP_BUFFER, 1);
		t.rx[t.rxc++] = out++;

		/* restore state after last batch */
		if (count == 0)
			q_ap_write(&t, AHB_CSW,
				AHB_CSW_MDEBUG | AHB_CSW_PRIV |
				AHB_CSW_DBG_EN | AHB_CSW_32BIT);

		if (q_exec(&t))
			return -1;
	}
	return 0;
}

static int _swdp_ahb_write32(u32 addr, u32 *in, int count) {
	struct txn t;

	while (count > 0) {
		int xfer;

		/* auto-inc wraps at 4K page boundaries -- limit max
		 * transfer so we won't cross a page boundary
		 */
		xfer = (0x1000 - (addr & 0xFFF)) / 4;
		if (xfer > count)
			xfer = count;
		if (xfer > MAXDATAWORDS)
			xfer = MAXDATAWORDS;

		count -= xfer;
		q_init(&t);

		/* setup before initial txn */
		q_ap_write(&t, AHB_CSW,
			AHB_CSW_MDEBUG | AHB_CSW_PRIV | AHB_CSW_INC_SINGLE |
			AHB_CSW_DBG_EN | AHB_CSW_32BIT);

		/* initial address */
		q_ap_write(&t, AHB_TAR, addr);

		t.tx[t.txc++] = SWD_WR(OP_AP | (AHB_DRW & 0xC), xfer);
		addr += xfer * 4;
		while (xfer-- > 0) 
			t.tx[t.txc++] = *in++;

		/* restore state after last batch */
		if (count == 0)
			q_ap_write(&t, AHB_CSW,
				AHB_CSW_MDEBUG | AHB_CSW_PRIV |
				AHB_CSW_DBG_EN | AHB_CSW_32BIT);

		if (q_exec(&t))
			return -1;
	}
	return 0;
}
#endif

#if 0
int swdp_core_write(u32 n, u32 v) {
	struct txn t;
	q_init(&t);
	q_ahb_write(&t, CDBG_REG_DATA, v);
	q_ahb_write(&t, CDBG_REG_ADDR, (n & 0x1F) | 0x10000);
	return q_exec(&t);
}

int swdp_core_read(u32 n, u32 *v) {
	struct txn t;
	q_init(&t);
	q_ahb_write(&t, CDBG_REG_ADDR, n & 0x1F);
	q_ahb_read(&t, CDBG_REG_DATA, v);
	return q_exec(&t);
}

int swdp_core_read_all(u32 *v) {
	struct txn t;
	unsigned n;
	q_init(&t);
	for (n = 0; n < 19; n++) {
		q_ahb_write(&t, CDBG_REG_ADDR, n & 0x1F);
		q_ahb_read(&t, CDBG_REG_DATA, v++);
	}
	return q_exec(&t);
}

int swdp_step_no_ints = 0;

int swdp_core_halt(void) {
	u32 x;
	if (swdp_ahb_read(CDBG_CSR, &x)) return -1;
	x &= (CDBG_C_HALT | CDBG_C_DEBUGEN | CDBG_C_MASKINTS);
	x |= CDBG_CSR_KEY | CDBG_C_DEBUGEN | CDBG_C_HALT;
	return swdp_ahb_write(CDBG_CSR, x);
}

int swdp_core_step(void) {
	u32 x;
	if (swdp_ahb_read(CDBG_CSR, &x)) return -1;
	x &= (CDBG_C_HALT | CDBG_C_DEBUGEN | CDBG_C_MASKINTS);
	x |= CDBG_CSR_KEY;

	if (!(x & CDBG_C_HALT)) {
		// HALT if we're not already HALTED
		x |= CDBG_C_HALT | CDBG_C_DEBUGEN;
		swdp_ahb_write(CDBG_CSR, x);
	}
	if (swdp_step_no_ints) {
		// set MASKINTS if not already set
		if (!(x & CDBG_C_MASKINTS)) {
			x |= CDBG_C_MASKINTS;
			swdp_ahb_write(CDBG_CSR, x);
		}
	} else {
		// clear MASKINTs if not already clear
		if (x & CDBG_C_MASKINTS) {
			x &= (~CDBG_C_MASKINTS);
			swdp_ahb_write(CDBG_CSR, x);
		}
	}
	// STEP
	x &= (~CDBG_C_HALT);
	return swdp_ahb_write(CDBG_CSR, x | CDBG_C_STEP);
}

int swdp_core_resume(void) {
	u32 x;
	if (swdp_ahb_read(CDBG_CSR, &x)) return -1;
	x &= (CDBG_C_HALT | CDBG_C_DEBUGEN | CDBG_C_MASKINTS);
	x |= CDBG_CSR_KEY | CDBG_C_DEBUGEN;

	if (swdp_step_no_ints > 1) {
		// not just on during step, but always
		if (!(x & CDBG_C_MASKINTS)) {
			x |= CDBG_C_MASKINTS;
			swdp_ahb_write(CDBG_CSR, x);
		}
	} else {
		if (x & CDBG_C_MASKINTS) {
			x &= (~CDBG_C_MASKINTS);
			swdp_ahb_write(CDBG_CSR, x);
		}
	}

	x &= ~(CDBG_C_HALT | CDBG_C_STEP);
	return swdp_ahb_write(CDBG_CSR, x);
}

int swdp_core_wait_for_halt(void) {
	int last = ATTN;
	u32 csr;
	for (;;) {
		if (swdp_ahb_read(CDBG_CSR, &csr))
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
		if (swdp_ahb_read(addr, &val))
			return -1;
		if (ATTN != last)
			return -2;
	} while (val == oldval);
	return 0;
}

int swdp_watchpoint(unsigned n, u32 addr, u32 func) {
	struct txn t;

	if (n > 3)
		return -1;

	q_init(&t);
	/* enable DWT, enable all exception traps */
	q_ahb_write(&t, DEMCR, DEMCR_TRCENA | DEMCR_VC_CORERESET);
	q_ahb_write(&t, DWT_FUNC(n), DWT_FN_DISABLED);
	if (func != DWT_FN_DISABLED) {
		q_ahb_write(&t, DWT_COMP(n), addr);
		q_ahb_write(&t, DWT_MASK(n), 0);
		q_ahb_write(&t, DWT_FUNC(n), func);
	}
	return q_exec(&t);
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
#endif

int swdp_bootloader(void) {
	struct txn t;
	q_init(&t);
	t.tx[t.txc++] = RSWD_MSG(CMD_BOOTLOADER, 0, 0);
	return q_exec(&t);
}

static int _swdp_reset(void) {
	struct txn t;
	u32 n, idcode;

	swd_error = 0;
	q_init(&t);
	t.tx[t.txc++] = RSWD_MSG(CMD_ATTACH, 0, 0);
	t.tx[t.txc++] = SWD_RD(DP_IDCODE, 1);
	t.rx[t.rxc++] = &idcode;
	if (q_exec(&t)) {
		xprintf(XSWD, "attach: IDCODE: ????????\n");
	} else {
		xprintf(XSWD, "attach: IDCODE: %08x\n", idcode);
	}

	swd_error = 0;
	q_init(&t);
 	/* clear any stale errors */
	t.tx[t.txc++] = SWD_WR(DP_ABORT, 1);
	t.tx[t.txc++] = 0x1E;

	/* power up */
	t.tx[t.txc++] = SWD_WR(DP_DPCTRL, 1);
	t.tx[t.txc++] = (1 << 28) | (1 << 30);
	t.tx[t.txc++] = SWD_RD(DP_DPCTRL, 1);
	t.rx[t.rxc++] = &n;

	/* configure for 32bit IO */
	q_ap_write(&t, AHB_CSW,
		AHB_CSW_MDEBUG | AHB_CSW_PRIV |
		AHB_CSW_PRIV | AHB_CSW_DBG_EN | AHB_CSW_32BIT);
	if (q_exec(&t))
		return -1;

	xprintf(XSWD, "attach: DPCTRL: %08x\n", n);
	return 0;
}

static int _swdp_clear_error(void) {
	if (swd_error == 0) {
		return 0;
	} else {
		struct txn t;
		swd_error = 0;

		q_init(&t);
		t.tx[t.txc++] = SWD_WR(DP_ABORT, 1);
		t.tx[t.txc++] = 0x1E;
		q_exec(&t);

		return swd_error;
	}
}

void swdp_enable_tracing(int yes) {
	struct txn t;
	q_init(&t);
	t.tx[t.txc++] = RSWD_MSG(CMD_TRACE, yes, 0);
	q_exec(&t);
}

void swdp_target_reset(int enable) {
	struct txn t;
	q_init(&t);
	t.tx[t.txc++] = RSWD_MSG(CMD_RESET, 0, enable);
	q_exec(&t);
}

int swdp_set_clock(unsigned khz) {
	struct txn t;
	if (khz > 0xFFFF)
		return -1;
	if (khz < 1000)
		khz = 1000;
	q_init(&t);
	t.tx[t.txc++] = RSWD_MSG(CMD_SET_CLOCK, 0, khz);
	return q_exec(&t);
}

int swo_set_clock(unsigned khz) {
	struct txn t;
	if (khz > 0xFFFF)
		return -1;
	if (khz < 1000)
		khz = 1000;
	q_init(&t);
	t.tx[t.txc++] = RSWD_MSG(CMD_SWO_CLOCK, 0, khz);
	return q_exec(&t);
}

int swdp_open(void) {
	pthread_create(&swd_thread, NULL, swd_reader, NULL);
	return 0;
}

int jtag_io(unsigned count, u32 *tms, u32 *tdi, u32 *tdo) {
	struct txn t;
	q_init(&t);
	if (count > 32768)
		return -1;
	t.tx[t.txc++] = RSWD_MSG(CMD_JTAG_IO, 0, count);
	count = (count + 31) / 32;
	while (count > 0) {
		t.tx[t.txc++] = *tms++;
		t.tx[t.txc++] = *tdi++;
		t.rx[t.rxc++] = tdo++;
		count--;
	}
	return q_exec(&t);
}

debug_transport SWDP_TRANSPORT = {
	.attach = _swdp_reset,
	.error = _swdp_error,
	.clear_error = _swdp_clear_error,
	.mem_rd_32 = _swdp_ahb_read,
	.mem_wr_32 = _swdp_ahb_write,
	.mem_rd_32_c = _swdp_ahb_read32,
	.mem_wr_32_c = _swdp_ahb_write32,
};

