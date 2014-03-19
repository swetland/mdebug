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
	unsigned addr = GPIODIR(0) | (n & 0x30000);
	n &= 0xFFF;
	if (cfg & GPIO_CFG_OUT) {
		writel(readl(addr) | n, addr);
	} else {
		writel(readl(addr) & (~n), addr);
	}
}

void gpio_cfg_irq(unsigned n, unsigned cfg) {
	unsigned off = (n & 0x30000);
	unsigned addr;
	n &= 0xFFF;

	addr = GPIOBOTHEDGES(0) + off;
	if (cfg & GPIO_CFG_BOTH) {
		writel(readl(addr) | n, addr);
	} else {
		writel(readl(addr) & (~n), addr);
		addr = GPIOPOLARITY(0) + off;
		if (cfg & GPIO_CFG_NEGATIVE) {
			writel(readl(addr) & (~n), addr);
		} else {
			writel(readl(addr) | n, addr);
		}
	}

	addr = GPIOLEVEL(0) + off;
	if (cfg & GPIO_CFG_EDGE) {
		writel(readl(addr) & (~n), addr);
	} else {
		writel(readl(addr) | n, addr);
	}
}

int gpio_irq_check(unsigned n) {
	unsigned off = (n & 0x30000);
	return (readl(GPIORAWISR(0) + off) & (n & 0xFFF)) != 0;
}

void gpio_irq_clear(unsigned n) {
	unsigned off = (n & 0x30000);
	writel(n & 0xFFF, GPIOICR(0) + off);
}

void gpio_set(unsigned n) {
	unsigned addr = GPIOBASE(0) | (n & 0x30000) | ((n & 0xFFF) << 2);
	writel(0xFFFFFFFF, addr);
}

void gpio_clr(unsigned n) {
	unsigned addr = GPIOBASE(0) | (n & 0x30000) | ((n & 0xFFF) << 2);
	writel(0, addr);
}

void gpio_wr(unsigned n, unsigned val) {
	unsigned addr = GPIOBASE(0) | (n & 0x30000) | ((n & 0xFFF) << 2);
	writel(val ? 0xFFFFFFFF : 0, addr);
}

int gpio_rd(unsigned n) {
	unsigned addr = GPIODATA(0) | (n & 0x30000);
	n &= 0xFFF;
	return (readl(addr) & n) != 0;
}
