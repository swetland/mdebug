/* swdp.c
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

#include <arch/hardware.h>

#include <protocol/rswdp.h>
#include "swdp.h"

unsigned swdp_trace = 0;

void printu(const char *fmt, ...);

/* NOTES
 * host -> device:
 *   host writes DATA on falling edge of CLOCK
 *   device reads DATA on rising edge of CLOCK
 * device -> host
 *   device writes DATA on rising edge of CLOCK
 *   host samples DATA on falling edge of CLOCK
 * host parks (begins turnaround) by:
 *   releasing bus between falling and rising clock
 *   a turnaround cycle follows, in which the device does its first write on CK+
 * host unparks (reclaims bus) by:
 *   reasserting bus between falling and rising clock
 */

#define GPIO_CLK 1
#define GPIO_DAT 0

#define D0 (1 << (GPIO_DAT + 16))
#define D1 (1 << GPIO_DAT)
#define C0 (1 << (GPIO_CLK + 16))
#define C1 (1 << (GPIO_CLK))

#define GPIOA (GPIOA_BASE + GPIO_BSR)

#define XMIT(data) writel(data | C0, GPIOA), writel(data | C1, GPIOA)

#define D_OUT() gpio_config(GPIO_DAT, GPIO_OUTPUT_10MHZ | GPIO_OUT_PUSH_PULL)
#define D_IN() gpio_config(GPIO_DAT, GPIO_INPUT | GPIO_PU_PD)

static unsigned recv(unsigned count) {
	unsigned n = 0;
	unsigned bit = 1;

	while (count-- > 0) {
		writel(D1 | C0, GPIOA);
		if (readl(GPIOA_BASE + GPIO_IDR) & (1 << GPIO_DAT))
			n |= bit;
		bit <<= 1;
		writel(D1 | C1, GPIOA);
	}
	return n;
}

static void send(unsigned n, unsigned count) {
	unsigned p = 0;
	while (count-- > 0) {
		p ^= (n & 1);
		if (n & 1) {
			XMIT(D1);
		} else {
			XMIT(D0);
		}	
		n >>= 1;
	}
	if (p) {
		XMIT(D1);
	} else {
		XMIT(D0);
	}
}

static void clock_high(int n) {
	while (n-- > 0)
		XMIT(D1);
}
static void clock_low(int n) {
	while (n-- > 0)
		XMIT(D0);
}
static void clock_jtag2swdp() {
	XMIT(D0); XMIT(D1); XMIT(D1); XMIT(D1);
	XMIT(D1); XMIT(D0); XMIT(D0); XMIT(D1);
	XMIT(D1); XMIT(D1); XMIT(D1); XMIT(D0);
	XMIT(D0); XMIT(D1); XMIT(D1); XMIT(D1);
}

static void puth(unsigned hdr, unsigned n, unsigned v) {
	printu("%s %s %b %s %x\n",
		(hdr & 0x20) ? "RD" : "WR",
		(hdr & 0x40) ? "AP" : "DP",
		(hdr >> 3) & 3,
		(n == 1) ? "OK" : "XX",
		v);
}

int swdp_read(unsigned hdr, unsigned *v) {
	unsigned n,m,o;

	gpio_clr(2);
	for (n = 0; n < 8; n++) {
		if (hdr & 0x80) {
			XMIT(D1);
		} else {
			XMIT(D0);
		}
		hdr <<= 1;
	}
	D_IN();
	XMIT(D1); // turnaround
	
	n = recv(3);
	m = recv(32);
	o = recv(1);
	D_OUT();
	XMIT(D1); // turnaround
	clock_low(8);
	gpio_set(2);

	if (swdp_trace || (n != 1))
		puth(hdr >> 8, n, m);

	*v = m;
	return (n == 1) ? 0 : -1;
}

int swdp_write(unsigned hdr, unsigned val) {
	unsigned n;

	for (n = 0; n < 8; n++) {
		if (hdr & 0x80) {
			XMIT(D1);
		} else {
			XMIT(D0);
		}
		hdr <<= 1;
	}
	D_IN();
	XMIT(D1); // turnaround
	
	n = recv(3);
	D_OUT();
	XMIT(D1);
	send(val, 32);
	clock_low(8);

	if (swdp_trace || (n != 1))
		puth(hdr >> 8, n, val);

	return (n == 1) ? 0 : -1;
}

void swdp_reset(void) {
	gpio_set(GPIO_CLK);
	gpio_set(GPIO_DAT);
	gpio_config(GPIO_CLK, GPIO_OUTPUT_10MHZ | GPIO_OUT_PUSH_PULL);
	gpio_config(GPIO_DAT, GPIO_OUTPUT_10MHZ | GPIO_OUT_PUSH_PULL);

	/* tracing */
	gpio_set(2);
	gpio_config(2, GPIO_OUTPUT_2MHZ | GPIO_OUT_PUSH_PULL);

	clock_high(64);
	clock_jtag2swdp();
	clock_high(64);
	clock_low(8);
}

