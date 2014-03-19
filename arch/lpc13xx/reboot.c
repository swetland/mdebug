/* reboot.c
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

void reboot(void) {
	/* select IRC */
	writel(0, 0x400480D0);
	/* enable */
	writel(0, 0x400480D4);
	writel(1, 0x400480D4);
	/* DIV = 1 */
	writel(1, 0x400480D8);
	writel(readl(SYS_CLK_CTRL) | SYS_CLK_WDT, SYS_CLK_CTRL);
	/* ENABLE and RESET */
	writel(3, 0x40004000);
	/* FEED */
	writel(0xAA, 0x40004008);
	writel(0x55, 0x40004008);
	for (;;) ;
}
