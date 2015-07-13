/* lkdebug.c
 *
 * Copyright 2015 Brian Swetland <swetland@frotz.net>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fw/types.h>

#include "rswdp.h"
#include "debugger.h"
#include "lkdebug.h"

#define DI_MAGIC	0x52474244
#define DI_OFF_MAGIC	32
#define DI_OFF_PTR	36

#define LK_THREAD_MAGIC	0x74687264
#define LIST_OFF_PREV	0
#define LIST_OFF_NEXT	4

typedef struct lkdebuginfo {
	u32 version;
	u32 thread_list_ptr;
	u32 current_thread_ptr;
	u8 off_list_node;
	u8 off_state;
	u8 off_saved_sp;
	u8 off_was_preempted;
	u8 off_name;
	u8 off_waitq;
} lkdebuginfo_t;

#define LK_MAX_STATE	5
static char *lkstate[] = {
	"SUSP ",
	"READY",
	"RUN  ",
	"BLOCK",
	"SLEEP",
	"DEAD ",
	"?????",
};

void dump_lk_thread(lkthread_t *t) {
	xprintf("thread: @%08x sp=%08x wq=%08x st=%d name='%s'\n",
		t->threadptr, t->saved_sp, t->waitq, t->state, t->name);
	xprintf("  r0 %08x r4 %08x r8 %08x ip %08x\n",
		t->regs[0], t->regs[4], t->regs[8], t->regs[12]);
	xprintf("  r1 %08x r5 %08x r9 %08x sp %08x\n",
		t->regs[1], t->regs[5], t->regs[9], t->regs[13]);
	xprintf("  r2 %08x r6 %08x 10 %08x lr %08x\n",
		t->regs[2], t->regs[6], t->regs[10], t->regs[14]);
	xprintf("  r3 %08x r7 %08x 11 %08x pc %08x\n",
		t->regs[3], t->regs[7], t->regs[11], t->regs[15]);
}

void dump_lk_threads(lkthread_t *t) {
	while (t != NULL) {
		dump_lk_thread(t);
		t = t->next;
	}
}

#define LT_NEXT_PTR(di,tp) ((tp) + di->off_list_node + LIST_OFF_NEXT)
#define LT_STATE(di,tp) ((tp) + di->off_state)
#define LT_SAVED_SP(di,tp) ((tp) + di->off_saved_sp)
#define LT_NAME(di,tp) ((tp) + di->off_name)
#define LT_WAITQ(di,tp) ((tp) + di->off_waitq)

#define LIST_TO_THREAD(di,lp) ((lp) - (di)->off_list_node)

static lkthread_t *read_lk_thread(lkdebuginfo_t *di, u32 ptr, int active) {
	lkthread_t *t = calloc(1, sizeof(lkthread_t));
	u32 x;
	int n;
	if (t == NULL) goto fail;
	t->threadptr = ptr;
	if (swdp_ahb_read(ptr, &x)) goto fail;
	if (x != LK_THREAD_MAGIC) goto fail;
	if (swdp_ahb_read(LT_NEXT_PTR(di,ptr), &t->nextptr)) goto fail;
	if (swdp_ahb_read(LT_STATE(di,ptr), &t->state)) goto fail;
	if (swdp_ahb_read(LT_SAVED_SP(di,ptr), &t->saved_sp)) goto fail;
	if (swdp_ahb_read(LT_WAITQ(di,ptr), &t->waitq)) goto fail;
	if (swdp_ahb_read32(LT_NAME(di,ptr), (void*) t->name, 32 / 4)) goto fail;
	t->name[31] = 0;
	for (n = 0; n < 31; n++) {
		if ((t->name[n] < ' ') || (t->name[n] > 127)) {
			if (t->name[n] == 0) break;
			t->name[n] = '.';
		}
	}
	if (t->state > LK_MAX_STATE) t->state = LK_MAX_STATE + 1;
	memset(t->regs, 0xee, sizeof(t->regs));
	// lk arm-m context frame: R4 R5 R6 R7 R8 R9 R10 R11 LR
	// if LR is FFFFFFxx then: R0 R1 R2 R3 R12 LR PC PSR
	t->active = active;
	if (!active) {
		u32 fr[9];
		if (swdp_ahb_read32(t->saved_sp, (void*) fr, 9)) goto fail;
		memcpy(t->regs + 4, fr, 8 * sizeof(u32));
		if ((fr[8] & 0xFFFFFF00) == 0xFFFFFF00) {
			if (swdp_ahb_read32(t->saved_sp + 9 * sizeof(u32), (void*) fr, 8)) goto fail;
			memcpy(t->regs + 0, fr, 4 * sizeof(u32));
			t->regs[12] = fr[4];
			t->regs[13] = t->saved_sp + 17 * sizeof(u32);
			t->regs[14] = fr[5];
			t->regs[15] = fr[6];
			t->regs[16] = fr[7];
		} else {
			t->regs[13] = t->saved_sp + 9 * sizeof(u32);
			t->regs[15] = fr[8];
			t->regs[16] = 0x10000000;
		}
	}
	return t;
fail:
	free(t);
	return NULL;
}

void free_lk_threads(lkthread_t *list) {
	lkthread_t *t, *next;
	for (t = list; t != NULL; t = next) {
		next = t->next;
		free(t);
	}
}

lkthread_t *find_lk_threads(int verbose) {
	lkdebuginfo_t di;
	lkthread_t *list = NULL;
	lkthread_t *current = NULL;
	lkthread_t *t;
	u32 x;
	u32 rtp;
	if (swdp_ahb_read(DI_OFF_MAGIC, &x)) goto fail;
	if (x != DI_MAGIC) {
		if (verbose) xprintf("debuginfo: bad magic\n");
		goto fail;
	}
	if (swdp_ahb_read(DI_OFF_PTR, &x)) goto fail;
	if (x & 3) goto fail;
	if (verbose) xprintf("debuginfo @ %08x\n", x);
	if (swdp_ahb_read32(x, (void*) &di, sizeof(di) / 4)) goto fail;
	if (verbose) {
		xprintf("di %08x %08x %08x %d %d %d %d %d %d\n",
			di.version, di.thread_list_ptr, di.current_thread_ptr,
			di.off_list_node, di.off_state, di.off_saved_sp,
			di.off_was_preempted, di.off_name, di.off_waitq);
	}
	if (di.version != 0x0100) {
		if (verbose) xprintf("debuginfo: unsupported version\n");
		goto fail;
	}
	if (swdp_ahb_read(di.current_thread_ptr, &x)) goto fail;
	current = read_lk_thread(&di, x, 1);
	if (current == NULL) goto fail;
	rtp = di.thread_list_ptr;
	for (;;) {
		if (swdp_ahb_read(rtp + LIST_OFF_NEXT, &rtp)) goto fail;
		if (rtp == di.thread_list_ptr) break;
		x = LIST_TO_THREAD(&di, rtp);
		if (current->threadptr == x) continue;
		t = read_lk_thread(&di, x, 0);
		if (t == NULL) goto fail;
		t->next = list;
		list = t;
	}
	current->next = list;
	return current;
fail:
	if (current) free(current);
	free_lk_threads(list);
	return NULL;
}

lkthread_t *find_lk_thread(lkthread_t *list, u32 tptr) {
	lkthread_t *t;
	for (t = list; t != NULL; t = t->next) {
		if (t->threadptr == tptr) {
			return t;
		}
	}
	return NULL;
}

void get_lk_thread_name(lkthread_t *t, char *out, size_t max) {
	snprintf(out, max, "%s - %s", lkstate[t->state], t->name);
}
