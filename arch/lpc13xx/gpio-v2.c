/* gpio.c
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

#include <fw/lib.h>
#include <fw/io.h>

#include <arch/hardware.h>

void gpio_cfg_dir(unsigned n, unsigned cfg) {
	unsigned addr = GPIO_DIR(GPIO_PORT(n));
	unsigned val = readl(addr);

	if (cfg & GPIO_CFG_OUT)
		val |= 1 << GPIO_NUM(n);
	else
		val &= ~(1 << GPIO_NUM(n));

	writel(val, addr);
}

void gpio_cfg_irq(unsigned n, unsigned cfg) {
	unsigned irq;
	irq = (cfg >> 8) & 0x7;

	if (cfg & GPIO_CFG_EDGE)
		clr_set_reg(GPIO_ISEL, 1 << irq, 0);
	else
		clr_set_reg(GPIO_ISEL, 0, 1 << irq);

	if (cfg & GPIO_CFG_POSITIVE)
		writel(1 << irq, GPIO_SIENR);
	else
		writel(1 << irq, GPIO_CIENR);


	if (cfg & GPIO_CFG_NEGATIVE)
		writel(1 << irq, GPIO_SIENF);
	else
		writel(1 << irq, GPIO_CIENF);

	writel(GPIO_PORT(n) * 24 + SCB_PINTSEL_INTPIN(GPIO_NUM(n)),
	       SCB_PINTSEL(irq));
}

int gpio_irq_check(unsigned n) {
	asm("b .");
	return 0;
#if 0
	unsigned off = (n & 0x30000);
	return (readl(GPIORAWISR(0) + off) & (n & 0xFFF)) != 0;
#endif
}

void gpio_irq_clear(unsigned n) {
	asm("b .");
#if 0
	unsigned off = (n & 0x30000);
	writel(n & 0xFFF, GPIOICR(0) + off);
#endif
}

void gpio_set(unsigned n) {
	writel(1 << GPIO_NUM(n), GPIO_SET(GPIO_PORT(n)));
}

void gpio_clr(unsigned n) {
	writel(1 << GPIO_NUM(n), GPIO_CLR(GPIO_PORT(n)));
}

void gpio_wr(unsigned n, unsigned val) {
	/* TODO: use the word set regs */
	if (val)
		gpio_set(n);
	else
		gpio_clr(n);
}

int gpio_rd(unsigned n) {
	return !!(readl(GPIO_W(GPIO_PORT(n) * 32 + GPIO_NUM(n))));
}
