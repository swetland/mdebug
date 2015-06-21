/* debugger.h
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

#ifndef _DEBUGGER_H_
#define _DEBUGGER_H_

#define printf __use_xprintf_in_debugger__
extern void xprintf(const char *fmt, ...);

#define ERROR		-1
#define ERROR_UNKNOWN 	-2

struct funcline {
	struct funcline *next;
	char text[0];
};

struct funcinfo {
	struct funcinfo *next;
	struct funcline *lines;
	char name[0];
};

struct varinfo {
	struct varinfo *next;
	u32 value;
	char name[0];
};

typedef struct {
	const char *s;
	unsigned n;
} param;

struct debugger_command {
	const char *name;
	const char *args;
	int (*func)(int argc, param *argv);
	const char *help;
};

/* provided by debugger-core.c */
int debugger_command(char *line);
int debugger_invoke(const char *cmd, unsigned argc, ...);
int debugger_variable(const char *name, u32 *value);

/* lock to protect underlying rswdp state */
void debugger_init();
void debugger_lock();
void debugger_unlock();

/* provided by debugger-commands.c */
extern struct debugger_command debugger_commands[];
int read_register(const char *name, u32 *value);
int read_memory_word(u32 addr, u32 *value);

#endif

