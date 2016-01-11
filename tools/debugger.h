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

typedef enum {
	XDEFAULT,
	XSWD,		// SWD transport & engine
	XCORE,		// debugger core
	XDATA,		// debugger command response
	XGDB,		// messages from GDB bridge
	XREMOTE,	// remote console messages
} xpchan;

#define printf __use_xprintf_in_debugger__
extern void xprintf(xpchan ch, const char *fmt, ...);

#define ERROR		-1
#define ERROR_UNKNOWN 	-2

#define LF_SWD		1
#define LF_GDB		2

extern unsigned log_flags;

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

typedef struct debug_transport {
	// attempt to establish connection to target
	int (*attach)(void);

	// returns nonzero if target is in error state
	// (one or more transactions have failed, attach needed)
	int (*error)(void);

	// if target is in error, clear error flag
	// return nonzero if target was in error (attach needed)
	int (*clear_error)(void);

	// single 32bit memory access
	int (*mem_rd_32)(u32 addr, u32 *value);
	int (*mem_wr_32)(u32 addr, u32 value);

	// multiple 32bit memory access
	int (*mem_rd_32_c)(u32 addr, u32 *data, int count);
	int (*mem_wr_32_c)(u32 addr, u32 *data, int count);
} debug_transport;

extern debug_transport *ACTIVE_TRANSPORT;

static inline int debug_attach(void) {
	return ACTIVE_TRANSPORT->attach();
}
static inline int debug_error(void) {
	return ACTIVE_TRANSPORT->error();
}
static inline int debug_clear_error(void) {
	return ACTIVE_TRANSPORT->clear_error();
}
static inline int mem_rd_32(u32 addr, u32 *value) {
	return ACTIVE_TRANSPORT->mem_rd_32(addr, value);
}
static inline int mem_wr_32(u32 addr, u32 value) {
	return ACTIVE_TRANSPORT->mem_wr_32(addr, value);
}
static inline int mem_rd_32_c(u32 addr, u32 *data, int count) {
	return ACTIVE_TRANSPORT->mem_rd_32_c(addr, data, count);
}
static inline int mem_wr_32_c(u32 addr, u32 *data, int count) {
	return ACTIVE_TRANSPORT->mem_wr_32_c(addr, data, count);
}

extern debug_transport DUMMY_TRANSPORT;
extern debug_transport SWDP_TRANSPORT;
extern debug_transport JTAG_TRANSPORT;

#endif

