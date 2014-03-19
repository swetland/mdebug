/* serial.c
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

static unsigned usart_calc_brr(unsigned pclk, unsigned baud)
{
	unsigned idiv, fdiv, tmp;
	idiv = ((25 * pclk) / (4 * baud));
	tmp = (idiv / 100) << 4;
	fdiv = idiv - (100 * (tmp >> 4));
	tmp |= ((((fdiv * 16) + 50) / 100)) & 0x0F;
	return tmp;
}

void serial_init(unsigned sysclk, unsigned baud) {
	writel(0, USART1_BASE + USART_CR1);
	writel(0, USART1_BASE + USART_CR2);
	writel(0, USART1_BASE + USART_CR3);
	writel(1, USART1_BASE + USART_GTPR); /* divide pclk by 1 */
	writel(usart_calc_brr(sysclk, baud), USART1_BASE + USART_BRR);
	writel(USART_CR1_ENABLE | USART_CR1_PARITY | USART_CR1_9BIT |
		USART_CR1_TX_ENABLE | USART_CR1_RX_ENABLE,
		USART1_BASE + USART_CR1);
}

void serial_putc(unsigned c) {
	while (!(readl(USART1_BASE + USART_SR) & USART_SR_TXE)) ;
	writel(c, USART1_BASE + USART_DR);
}

