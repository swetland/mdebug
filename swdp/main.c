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

#include <arch/hardware.h>

#include <protocol/rswdp.h>
#include "swdp.h"

#define TRACE 0

#define GPIO_LED	5
#define GPIO_RESET_N	3

extern unsigned swdp_trace;

void clocks_to_72mhz() {
	/* external oscillator enable */
	writel(readl(RCC_CR) | 0x00010001, RCC_CR);
	while ((readl(RCC_CR) & 0x00020000) == 0) ;

	/* flash prefetch enable */
	writel(0x12, 0x40022000);

	/* configure PLL for 72MHz */
	writel(readl(RCC_CFGR) | 0x001D0400, RCC_CFGR);
	writel(readl(RCC_CR) | 0x01000000, RCC_CR);
	while ((readl(RCC_CR) & 0x03000000) == 0) ;

	/* set SYSCLK to PLL */
	writel(readl(RCC_CFGR) | 2, RCC_CFGR);
	while ((readl(RCC_CFGR) & 8) == 0) ;
}

extern u8 __bss_start__;
extern u8 __bss_end__;
void bss_init() {
	u32 *bss, *end;
	bss = (void*) &__bss_start__;
	end = (void*) &__bss_end__;
	while (bss < end)
		*bss++ = 0;
}

#define DEBUG_MAX 52
static struct {
	u32 txn;
	u32 cmd;
	u8 data[DEBUG_MAX];
} pmsg = {
	.txn = RSWD_TXN_ASYNC,
	.cmd = RSWD_MSG(CMD_DEBUG_PRINT, 0, DEBUG_MAX/4),
};
static unsigned poff = 0;

void flushu(void) {
	while (poff < DEBUG_MAX)
		pmsg.data[poff++] = 0;
	usb_xmit(&pmsg, sizeof(pmsg));
	poff = 0;
}

void _putu(unsigned c) {
	pmsg.data[poff++] = c;
	if (poff == DEBUG_MAX)
		flushu();
}

void printu(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vprintx(_putu, fmt, ap);
	va_end(ap);
	if (poff)
		flushu();
}

static u8 optable[16] = {
	[OP_RD | OP_DP | OP_X0] = RD_IDCODE,
	[OP_RD | OP_DP | OP_X4] = RD_DPCTRL,
	[OP_RD | OP_DP | OP_X8] = RD_RESEND,
	[OP_RD | OP_DP | OP_XC] = RD_BUFFER,
	[OP_WR | OP_DP | OP_X0] = WR_ABORT,
	[OP_WR | OP_DP | OP_X4] = WR_DPCTRL,
	[OP_WR | OP_DP | OP_X8] = WR_SELECT,
	[OP_WR | OP_DP | OP_XC] = WR_BUFFER,
	[OP_RD | OP_AP | OP_X0] = RD_AP0,
	[OP_RD | OP_AP | OP_X4] = RD_AP1,
	[OP_RD | OP_AP | OP_X8] = RD_AP2,
	[OP_RD | OP_AP | OP_XC] = RD_AP3,
	[OP_WR | OP_AP | OP_X0] = WR_AP0,
	[OP_WR | OP_AP | OP_X4] = WR_AP1,
	[OP_WR | OP_AP | OP_X8] = WR_AP2,
	[OP_WR | OP_AP | OP_XC] = WR_AP3,
};

/* TODO bounds checking -- we trust the host far too much */
void process_txn(u32 txnid, u32 *rx, int rxc, u32 *tx) {
	unsigned msg, op, n;
	unsigned txc = 1;
	unsigned count = 0;
	unsigned status = 0;
	void (*func)(void) = 0;

	tx[0] = txnid;

	while (rxc-- > 0) {
		count++;
		msg = *rx++;
		op = RSWD_MSG_OP(msg);
		n = RSWD_MSG_ARG(msg);
#if TRACE
		printx("> %b %b %h <\n", RSWD_MSG_CMD(msg), op, n);
#endif
		switch (RSWD_MSG_CMD(msg)) {
		case CMD_NULL:
			continue;
		case CMD_SWD_WRITE:
			while (n-- > 0) {
				rxc--;
				if (swdp_write(optable[op], *rx++)) {
					status = 3;
					goto done;
				}
			}
			continue;
		case CMD_SWD_READ:
			tx[txc++] = RSWD_MSG(CMD_SWD_DATA, 0, n); 
			while (n-- > 0) {
				if (swdp_read(optable[op], tx + txc)) {
					txc++;
					while (n-- > 0)
						tx[txc++] = 0xfefefefe;
					status = 3;
					goto done;
				}
				txc++;
			}
			continue;
		case CMD_SWD_DISCARD:
			while (n-- > 0) {
				u32 tmp;
				if (swdp_read(optable[op], &tmp)) {
					status = 3;
					goto done;
				}
			}
			continue;
		case CMD_ATTACH:
			swdp_reset();
			continue;
		case CMD_RESET:
			if (n == 0) {
				/* deassert RESET */
				gpio_config(GPIO_RESET_N,
					GPIO_INPUT | GPIO_FLOATING);
			} else {
				/* assert RESET */
				gpio_clr(GPIO_RESET_N);
				gpio_config(GPIO_RESET_N,
					GPIO_OUTPUT_10MHZ | GPIO_ALT_PUSH_PULL);
			}
			continue;
		case CMD_DOWNLOAD: {
			u32 *addr = (void*) *rx++;
			rxc--;
			while (n) {
				*addr++ = *rx++;
				rxc--;
			}
			continue;
		}
		case CMD_EXECUTE:
			func = (void*) *rx++;
			rxc--;
			continue;
		case CMD_TRACE:
			swdp_trace = op;
			continue;
		default:
			printx("unknown command %b\n", RSWD_MSG_CMD(msg));
			status = 1;
			goto done;
		}
	}

done:
	tx[txc++] = RSWD_MSG(CMD_STATUS, status, count);
	if ((txc & 0x3f) == 0)
		tx[txc++] = RSWD_MSG(CMD_NULL, 0, 0);

#if TRACE
	printx("[ send %x words ]\n", txc);
	for (n = 0; n < txc; n+=4) {
		printx("%x %x %x %x\n",
			tx[n], tx[n+1], tx[n+2], tx[n+3]);
	}
#endif
	usb_xmit(tx, txc * 4);

	if (func) {
		func();
		for (;;) ;
	}	
}

static u32 rxbuffer[1024];
static u32 txbuffer[1024];

int main() {
	int rxc;

	writel(readl(RCC_APB2ENR) |
		RCC_APB2_GPIOA | RCC_APB2_USART1,
		RCC_APB2ENR);

	gpio_config(9, GPIO_OUTPUT_10MHZ | GPIO_ALT_PUSH_PULL);
	gpio_config(GPIO_LED, GPIO_OUTPUT_10MHZ | GPIO_OUT_PUSH_PULL);

	clocks_to_72mhz();
	bss_init();
	serial_init(72000000, 115200);
	printx("[ rswdp agent v0.9 ]\n");
	printx("[ built " __DATE__ " " __TIME__ " ]\n");

#if 0
	irq_set_base(0x20001000);
	irq_enable(i_usb_lp);
	irq_enable(i_usb_hp);
#endif

	usb_init(0x18d1, 0xdb03, 0, 0);

	for (;;) {
		gpio_clr(GPIO_LED);
		rxc = usb_recv(rxbuffer, sizeof(rxbuffer));
		gpio_set(GPIO_LED);

#if TRACE
		int n;
		printx("[ recv %x words ]\n", rxc/4);
		for (n = 0; n < (rxc/4); n+=4) {
			printx("%x %x %x %x\n",
				rxbuffer[n], rxbuffer[n+1], rxbuffer[n+2], rxbuffer[n+3]);
		}
#endif

		if ((rxc < 4) || (rxc & 3)) {
			printx("error, runt frame, or strange frame... %x\n", rxc);
			continue;
		}

		rxc = rxc / 4;

		if ((rxbuffer[0] & 0xFFFF0000) != 0xAA770000) {
			printx("invalid frame %x\n", rxbuffer[0]);
			continue;
		}

		process_txn(rxbuffer[0], rxbuffer + 1, rxc - 1, txbuffer);
	}
}
