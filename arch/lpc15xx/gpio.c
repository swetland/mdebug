/* gpio.c
 *
 * Copyright 2014 Brian Swetland <swetland@frotz.net>
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

#include <fw/io.h>
#include <fw/lib.h>
#include <arch/hardware.h>

#define GPIO_OFF(n) ((n >> 5) << 2)
#define GPIO_BIT(n) (1 << ((n) & 31))

void gpio_cfg_dir(unsigned n, unsigned dir) {
	u32 r = GPIO0_DIR + GPIO_OFF(n);
	if (dir == GPIO_CFG_OUT) {
		writel(readl(r) | GPIO_BIT(n), r);
	} else {
		writel(readl(r) & (~GPIO_BIT(n)), r);
	}
}

void gpio_set(unsigned n) {
	writeb(1, GPIO_BYTE(n));
}

void gpio_clr(unsigned n) {
	writeb(0, GPIO_BYTE(n));
}

int gpio_rd(unsigned n) {
	return readb(GPIO_BYTE(n));
}

void gpio_wr(unsigned n, unsigned v) {
	writeb(!!v, GPIO_BYTE(n));
}
