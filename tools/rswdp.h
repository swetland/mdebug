/* rswdp.h
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

#ifndef _RSWDP_H__
#define _RSWDP_H__

int swdp_ahb_read(u32 addr, u32 *value);
int swdp_ahb_write(u32 addr, u32 value);

/* bulk reads/writes (more efficient after ~3-4 words */
int swdp_ahb_read32(u32 addr, u32 *out, int count);
int swdp_ahb_write32(u32 addr, u32 *out, int count);

/* return 0 when *addr != oldval, -1 on error, -2 on interrupt */
int swdp_ahb_wait_for_change(u32 addr, u32 oldval);

int swdp_core_halt(void);
int swdp_core_step(void);
int swdp_core_resume(void);

/* return 0 when CPU halts, -1 if an error occurs, or -2 if interrupted */
int swdp_core_wait_for_halt(void);
void swdp_interrupt(void);

/* access to CPU registers */
int swdp_core_read(u32 n, u32 *v);
int swdp_core_read_all(u32 *v);
int swdp_core_write(u32 n, u32 v);

int swdp_watchpoint_pc(unsigned n, u32 addr);
int swdp_watchpoint_rd(unsigned n, u32 addr);
int swdp_watchpoint_wr(unsigned n, u32 addr);
int swdp_watchpoint_rw(unsigned n, u32 addr);

/* attempt to clear any error state from previous transactions */
/* return 0 if successful (or no error state existed) */
int swdp_clear_error(void);
int swdp_error(void);

int swdp_reset(void);

int swdp_open(void);

void swdp_enable_tracing(int yes);

void swdp_target_reset(int enable);

int swdp_bootloader(void);
int swdp_set_clock(unsigned khz);

#endif

