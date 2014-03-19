/* nvic.c
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

#define NVIC_ICTR		0xE000E004
#define NVIC_SET_ENABLE		0xE000E100
#define NVIC_CLR_ENABLE		0xE000E180
#define NVIC_SET_PENDING	0xE000E200
#define NVIC_CLR_PENDING	0xE000E280
#define NVIC_ACTIVE		0xE000E300
#define NVIC_PRIORITY		0xE000E400
#define NVIC_CPUID		0xE000ED00
#define NVIC_ICSR		0xE000ED04
#define NVIC_VTOR		0xE000ED08
#define NVIC_ARCR		0xE000ED0C
#define NVIC_SYS_CTL		0xE000ED10
#define NVIC_CFG_CTL		0xE000ED14
#define NVIC_HANDLER_PRIORITY	0xE000ED18
#define NVIC_SW_IRQ_TRIGGER	0xE000EF00

void irq_enable(unsigned n) {
	writel(1 << (n & 31), NVIC_SET_ENABLE + (n >> 5) * 4);
}

void irq_disable(unsigned n) {
	writel(1 << (n & 31), NVIC_CLR_ENABLE + (n >> 5) * 4);
}

void irq_assert(unsigned n) {
	writel(1 << (n & 31), NVIC_SET_PENDING + (n >> 5) * 4);
}

void irq_deassert(unsigned n) {
	writel(1 << (n & 31), NVIC_CLR_PENDING + (n >> 5) * 4);
}

void irq_set_prio(unsigned n, unsigned p) {
	unsigned shift = (n & 3) * 8;
	unsigned reg = NVIC_PRIORITY + (n >> 2) * 4;
	unsigned val = readl(reg);
	val = val & (~(0xFF << shift));
	val = val | ((n & 0xFF) << shift);
	writel(val, reg);
}

void irq_set_base(unsigned n) {
	writel(n, NVIC_VTOR);
}

