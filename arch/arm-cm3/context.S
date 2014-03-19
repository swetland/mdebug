/* context.S
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

.syntax unified

.global context_init
.global handle_m3_svc
.global handle_m3_pendsv

/* context_init(void (*entry)(void), u32 new_psp, u32_new_msp); 
 * - setup stack pointers
 * - switch to PSP, remaining in Priv mode
 * - jump to entrypoint
 */
context_init:
	msr PSP, r1
	msr MSP, r2
	mrs r3, CONTROL
	orr r3, r3, #2
	msr CONTROL, r3
	bx r0
	b .

handle_m3_pendsv:
	/* obtain pointer to global state */
	ldr r12, =(CONFIG_STACKTOP - 0x10)
	/* safely make current_thread = next_thread */
	cpsid i
	ldr r1, [r12, #4]
	ldr r0, [r12, #0]
	str r1, [r12, #0]
	cpsie i
	/* save previous thread state */
	mrs r2, psp
	stmdb r2!, {r4-r11}
	str r2, [r0, #0]
	/* restore new thread state */
	ldr r3, [r1, #0]
	ldmia r3!, {r4-r11}
	msr psp, r3
	bx lr

