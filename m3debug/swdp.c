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
#include <fw/io.h>
#include <fw/lib.h>

#include <arch/hardware.h>
#include <protocol/rswdp.h>

volatile unsigned count;

unsigned data[8];

static inline void DIR_OUT(unsigned n) {
	writel(SSP_CR0_BITS(n) | SSP_CR0_FRAME_SPI |
		SSP_CR0_CLK_HIGH | SSP_CR0_PHASE1 |
		SSP_CR0_CLOCK_RATE(1),
		SSP0_CR0);
	writel(IOCON_FUNC_1 | IOCON_DIGITAL, IOCON_PIO0_9); /* MOSI */
}

static inline void DIR_IN(unsigned n) {
	writel(IOCON_FUNC_0 | IOCON_DIGITAL, IOCON_PIO0_9); /* MOSI */
	writel(SSP_CR0_BITS(n) | SSP_CR0_FRAME_SPI |
		SSP_CR0_CLK_HIGH | SSP_CR0_PHASE0 |
		SSP_CR0_CLOCK_RATE(1),
		SSP0_CR0);
}

static inline void XMIT(unsigned n) {
	writel(n, SSP0_DR);
	while (readl(SSP0_SR) & SSP_BUSY);
	readl(SSP0_DR);
}

static inline unsigned RECV(void) {
	writel(0, SSP0_DR);
	while (readl(SSP0_SR) & SSP_BUSY);
	return readl(SSP0_DR);
}

void swdp_reset(void) {
	/* clock out 64 1s */
	DIR_OUT(16);
	writel(0xFFFF, SSP0_DR);
	writel(0xFFFF, SSP0_DR);
	writel(0xFFFF, SSP0_DR);
	writel(0xFFFF, SSP0_DR);
	while (readl(SSP0_SR) & SSP_BUSY) ;
	readl(SSP0_DR);
	readl(SSP0_DR);
	readl(SSP0_DR);
	readl(SSP0_DR);

	/* clock out 16bit init sequence */
	writel(0b0111100111100111, SSP0_DR);
	while (readl(SSP0_SR) & SSP_BUSY) ;
	readl(SSP0_DR);

	/* clock out 64 1s */
	writel(0xFFFF, SSP0_DR);
	writel(0xFFFF, SSP0_DR);
	writel(0xFFFF, SSP0_DR);
	writel(0xFFFF, SSP0_DR);
	while (readl(SSP0_SR) & SSP_BUSY) ;
	readl(SSP0_DR);
	readl(SSP0_DR);
	readl(SSP0_DR);
	readl(SSP0_DR);
}

static unsigned revbits(unsigned n) {
        n = ((n >>  1) & 0x55555555) | ((n <<  1) & 0xaaaaaaaa);
        n = ((n >>  2) & 0x33333333) | ((n <<  2) & 0xcccccccc);
        n = ((n >>  4) & 0x0f0f0f0f) | ((n <<  4) & 0xf0f0f0f0);
        n = ((n >>  8) & 0x00ff00ff) | ((n <<  8) & 0xff00ff00);
        n = ((n >> 16) & 0x0000ffff) | ((n << 16) & 0xffff0000);
        return n;
}

/* returns 1 if the number of bits set in n is odd */
static unsigned parity(unsigned n) {
        n = (n & 0x55555555) + ((n & 0xaaaaaaaa) >> 1);
        n = (n & 0x33333333) + ((n & 0xcccccccc) >> 2);
        n = (n & 0x0f0f0f0f) + ((n & 0xf0f0f0f0) >> 4);
        n = (n & 0x00ff00ff) + ((n & 0xff00ff00) >> 8);
        n = (n & 0x0000ffff) + ((n & 0xffff0000) >> 16);
        return n & 1;
}

volatile int count_rd_wait = 0;
volatile int count_wr_wait = 0;

int swdp_write(unsigned n, unsigned v) {
	unsigned a, p;

again:
	/* clock out 8 0s and read sequence */
	XMIT(n);

	/* read X R0 R1 R2 */
	DIR_IN(4);
	a = RECV();
	if ((a & 7) != 4) {
		DIR_OUT(16);
		if ((a & 7) == 2) {
			count_wr_wait++;
			goto again;
		}
		return -1;
	}

	p = parity(v);
	v = revbits(v);

	/* transmit X D0..D31 P */
	DIR_OUT(9);
	XMIT(0x100 | (v >> 24));
	DIR_OUT(16);
	XMIT(v >> 8);
	DIR_OUT(9);
	XMIT((v << 1) | p);

	DIR_OUT(16);

	return 0;
}

int swdp_read(unsigned n, unsigned *out) {
	unsigned a, b, c;

again:
	/* clock out 8 0s and read sequence */
	XMIT(n);

	/* read X R0 R1 R2 */
	DIR_IN(4);
	a = RECV();
	if ((a & 7) != 4) {
		DIR_OUT(16);
		if ((a & 7) == 2) {
			count_rd_wait++;
			goto again;
		}
		*out = 0xffffffff;
		return -1;
	}

	/* D0..D31 P X */
	DIR_IN(16);
	a = RECV();
	DIR_IN(8);
	b = RECV();
	DIR_IN(10);
	c = RECV();

	*out = revbits((a << 16) | (b << 8) | (c >> 2));
	DIR_OUT(16);
	return 0;
}
