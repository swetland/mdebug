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

#include <fw/types.h>
#include <fw/lib.h>
#include <fw/io.h>

#include <arch/hardware.h>

static unsigned gpio_cr[4] = {
	GPIOA_BASE + GPIO_CRL,
	GPIOA_BASE + GPIO_CRH,
	GPIOB_BASE + GPIO_CRL,
	GPIOB_BASE + GPIO_CRH,
};

void gpio_config(unsigned n, unsigned cfg)
{
	unsigned addr = gpio_cr[n >> 3];
	unsigned shift = (n & 7) * 4;
	unsigned val = readl(addr);
	val = (val & (~(0xF << shift))) | (cfg << shift);
	writel(val, addr);
}

void gpio_set(unsigned n)
{
	unsigned addr = (n > 15) ? GPIOB_BASE : GPIOA_BASE;
	writel(1 << (n & 15), addr + GPIO_BSR);
}

void gpio_clr(unsigned n)
{
	unsigned addr = (n > 15) ? GPIOB_BASE : GPIOA_BASE;
	writel(1 << (n & 15), addr + GPIO_BRR);
}

