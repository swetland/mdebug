/* context.h
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

#ifndef _ARM_M3_CONTEXT_H_
#define _ARM_M3_CONTEXT_H_

struct thread {
	void *sp;
	u32 state;
	struct thread *next;
	struct thread *prev;
};

struct global_context {
	struct thread *current_thread;
	struct thread *next_thread;
	u32 unused0;
	u32 unused1;
};

#define GLOBAL_CONTEXT ((struct global_context *) (CONFIG_STACKTOP - 0x10))

/* state saved on the stack by hw on handler entry */
struct hw_state {
	u32 r0;
	u32 r1;
	u32 r2;
	u32 r3;
	u32 r12;
	u32 lr;
	u32 pc;
	u32 psr;
};

struct thread_state {
	/* saved by software */
	u32 r4;
	u32 r5;
	u32 r6;
	u32 r7;
	u32 r8;
	u32 r9;
	u32 r10;
	u32 r11;
	/* saved by hardware */
	u32 r0;
	u32 r1;
	u32 r2;
	u32 r3;
	u32 r12;
	u32 lr;
	u32 pc;
	u32 psr;
};

void context_init(void (*entry)(void), u32 psp, u32 msp);

#endif
