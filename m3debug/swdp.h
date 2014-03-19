/* swdp.h 
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

#ifndef _SWDP_H_
#define _SWDP_H_

void swdp_reset(void);
int swdp_write(unsigned reg, unsigned val);
int swdp_read(unsigned reg, unsigned *val);

unsigned swdp_ap_write(unsigned reg, unsigned val);
unsigned swdp_ap_read(unsigned reg);

unsigned swdp_ahb_read(unsigned addr);
void swdp_ahb_write(unsigned addr, unsigned value);

/* swdp_read/write() register codes */
#define RD_IDCODE	0xA5	// 10100101
#define RD_DPCTRL	0xB1	// 10110001
#define RD_RESEND	0xA9	// 10101001
#define RD_BUFFER	0xBD	// 10111101

#define WR_ABORT	0x81	// 10000001
#define WR_DPCTRL	0x95	// 10010101
#define WR_SELECT	0x8D	// 10001101
#define WR_BUFFER	0x99	// 10011001

#define RD_AP0		0xE1	// 11100001
#define RD_AP1		0xF5	// 11110101
#define RD_AP2		0xED	// 11101101
#define RD_AP3		0xF9	// 11111001

#define WR_AP0		0xC5	// 11000101
#define WR_AP1		0xD1	// 11010001
#define WR_AP2		0xC9	// 11001001
#define WR_AP3		0xDD	// 11011101

#endif

