/* swo.c
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
#include <unistd.h>
#include <string.h>

#include <pthread.h>
#include <fw/types.h>
#include <debugger.h>

#define TRACE 0

static char console_data[256];
static int console_ptr = 0;

static void handle_swv_src(unsigned id, unsigned val, unsigned n) {
#if TRACE
	xprintf(XDATA, "SRC %s %02x %08x\n", (id & 0x100) ? "HW" : "SW", id & 0xFF, val);
#endif
	if (id == 0) {
		console_data[console_ptr++] = val;
		if (val == '\n') {
			console_data[console_ptr] = 0;
			xprintf(XDATA, "[remote] %s", console_data);
			console_ptr = 0;
		} else if (console_ptr == 254) {
			console_data[console_ptr] = 0;
			xprintf(XDATA, "[remote] %s\n", console_data);
		}
	}
}

static void handle_swv_proto(unsigned char *data, unsigned len) {
#if TRACE
	switch (len) {
	case 1:
		xprintf(XDATA, "PRO %02x\n", data[0]);
		break;
	case 2:
		xprintf(XDATA, "PRO %02x %02x\n", data[0], data[1]);
		break;
	case 3:
		xprintf(XDATA, "PRO %02x %02x %02x\n",
			data[0], data[1], data[2]);
		break;
	case 4:
		xprintf(XDATA, "PRO %02x %02x %02x %02x\n",
			data[0], data[1], data[2], data[3]);
		break;
	case 5:
		xprintf(XDATA, "PRO %02x %02x %02x %02x %02x\n",
			data[0], data[1], data[2], data[3], data[4]);;
		break;
	case 6:
		xprintf(XDATA, "PRO %02x %02x %02x %02x %02x %02x\n",
			data[0], data[1], data[2], data[3],
			data[4], data[5]);
		break;
	case 7:
		xprintf(XDATA, "PRO %02x %02x %02x %02x %02x %02x %02x\n",
			data[0], data[1], data[2], data[3],
			data[4], data[5], data[6]);
		break;
	}
#endif
}

typedef enum {
	SWV_SYNC,
	SWV_1X1,
	SWV_1X2,
	SWV_1X4,
	SWV_2X2,
	SWV_2X4,
	SWV_3X4,
	SWV_4X4,
	SWV_PROTO,
	SWV_IDLE,
} swv_state_t;

typedef struct {
	swv_state_t state;
	unsigned zcount;
	unsigned ccount;
	unsigned id;
	unsigned val;
	unsigned char data[8];
} swv_t;

static swv_t swv = {
	.state = SWV_SYNC,
	.zcount = 0
};

static void handle_swv(swv_t *swv, unsigned x) {
	// any sequence ending in 00 00 00 00 00 80 is a re-sync
	if (x == 0) {
		swv->zcount++;
	} else {
		if ((swv->zcount >= 5) && (x == 0x80)) {
			swv->state = SWV_IDLE;
			swv->zcount = 0;
#if TRACE
			xprintf(XDATA, "SYNC\n");
#endif
			return;
		}
		swv->zcount = 0;
	}

	switch (swv->state) {
	case SWV_IDLE:
		if (x & 7) {
			// AAAAAHSS source packet
			swv->id = (x >> 3) | ((x & 4) << 6);
			swv->val = 0;
			swv->state = (x & 3);
		} else if (x != 0) {
			// CXXXXX00 protocol packet
			swv->data[0] = x;
			if (x & 0x80) {
				swv->ccount = 1;
				swv->state = SWV_PROTO;
			} else {
				handle_swv_proto(swv->data, 1);
			}
		} else {
			// 00 packets are for sync, ignore
		}
		break;
	case SWV_PROTO:
		swv->data[swv->ccount++] = x;
		// protocol packets end at 7 total bytes or a byte with bit7 clear
		if ((swv->ccount == 7) || (!(x & 0x80))) {
			handle_swv_proto(swv->data, swv->ccount);
			swv->state = SWV_IDLE;
		}
		break;
	case SWV_1X1:
		handle_swv_src(swv->id, x, 1);
		swv->state = SWV_IDLE;
		break;
	case SWV_1X2:
		swv->val = x;
		swv->state = SWV_2X2;
		break;
	case SWV_2X2:
		handle_swv_src(swv->id, swv->val | (x << 8), 2);
		swv->state = SWV_IDLE;
		break;
	case SWV_1X4:
		swv->val = x;
		swv->state = SWV_2X4;
		break;
	case SWV_2X4:
		swv->val |= (x << 8);
		swv->state = SWV_3X4;
		break;
	case SWV_3X4:
		swv->val |= (x << 16);
		swv->state = SWV_4X4;
		break;
	case SWV_4X4:
		handle_swv_src(swv->id, swv->val | (x << 24), 4);
		swv->state = SWV_IDLE;
		break;
	case SWV_SYNC:
		break;
	default:
		// impossible
		xprintf(XDATA, "fatal error, bad state %d\n", swv->state);
		exit(1);
	}
}

void process_swo_data(void *_data, unsigned count) {
	unsigned char *data = _data;
	while(count-- > 0) {
		handle_swv(&swv, *data++);
	}
}

