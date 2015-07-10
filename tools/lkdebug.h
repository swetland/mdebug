/* lkdebug.h
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

#ifndef _LKDEBUG_H_
#define _LKDEBUG_H_

typedef struct lkthread {
	struct lkthread *next;
	int active;
	u32 threadptr;
	u32 nextptr;
	u32 state;
	u32 saved_sp;
	u32 preempted;
	u32 waitq;
	char name[32];
	u32 regs[17];
} lkthread_t;

// probe target for lk thread info
lkthread_t *find_lk_threads(int verbose);

void dump_lk_thread(lkthread_t *t);
void dump_lk_threads(lkthread_t *t);
void free_lk_threads(lkthread_t *list);
lkthread_t *find_lk_thread(lkthread_t *list, u32 tptr);
void get_lk_thread_name(lkthread_t *t, char *out, size_t max);

#endif
