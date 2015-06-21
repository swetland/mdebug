/* debugger-core.c
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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include <fw/types.h>
#include "debugger.h"

#include <pthread.h>

#include "rswdp.h"

// for core debug regs
#include <protocol/rswdp.h>

#include "linenoise.h"

#define DHCSR_C_DEBUGEN		(1 << 0)
#define DHCSR_C_HALT		(1 << 1)
#define DHCSR_C_STEP		(1 << 2)
#define DHCSR_C_MASKINTS	(1 << 3)
#define DHCSR_C_SNAPSTALL	(1 << 5)
#define DHCSR_S_REGRDY		(1 << 16)
#define DHCSR_S_HALT		(1 << 17)
#define DHCSR_S_SLEEP		(1 << 18)
#define DHCSR_S_LOCKUP		(1 << 19)
#define DHCSR_S_RETIRE_ST	(1 << 24)
#define DHCSR_S_RESET_ST	(1 << 25)

#define DFSR			0xE000ED30
#define DFSR_HALTED		(1 << 0)
#define DFSR_BKPT		(1 << 1)
#define DFSR_DWTTRAP		(1 << 2)
#define DFSR_VCATCH		(1 << 3)
#define DFSR_EXTERNAL		(1 << 4)
#define DFSR_MASK		0x1F

#define DEBUG_MONITOR 1

#if DEBUG_MONITOR

static void m_event(const char *evt) {
	linenoisePause();
	fprintf(stdout, "DEBUG EVENT: %s\n", evt);
	linenoiseResume();
}

static void monitor(void) {
	u32 v;
	if (swdp_clear_error()) return;
	if (swdp_ahb_read(DFSR, &v) == 0) {
		if (v & DFSR_MASK) {
			swdp_ahb_write(DFSR, DFSR_MASK);
		}
		if (v & DFSR_HALTED) m_event("HALTED");
		if (v & DFSR_BKPT) m_event("BKPT");
		if (v & DFSR_DWTTRAP) m_event("DWTTRAP");
		if (v & DFSR_VCATCH) m_event("VCATCH");
		if (v & DFSR_EXTERNAL) m_event("EXTERNAL");
	}
}

static pthread_mutex_t _dbg_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t _dbg_thread;

void debugger_lock() {
	pthread_mutex_lock(&_dbg_lock);
	if (swdp_clear_error()) {
		xprintf("SWD ERROR persists. Attempting link reset.\n");
		swdp_reset();
	}
}

void debugger_unlock() {
	if (swdp_error()) {
		xprintf("SWD ERROR\n");
	}
	pthread_mutex_unlock(&_dbg_lock);
}

void *debugger_monitor(void *arg) {
	for (;;) {
		pthread_mutex_lock(&_dbg_lock);
		monitor();
		pthread_mutex_unlock(&_dbg_lock);
		usleep(250000);
	}
}

void debugger_init() {
	pthread_create(&_dbg_thread, NULL, debugger_monitor, NULL);
}

#else

void debugger_lock() {
	swdp_clear_error();
}
void debugger_unlock() {
}
void debugger_init() {
}

#endif


static struct varinfo *all_variables = 0;
static struct funcinfo *allfuncs = 0;

static int frame_argc = 0;
static param *frame_argv = 0;

static void variable_set(const char *name, u32 value) {
	struct varinfo *vi;
	int len;
	for (vi = all_variables; vi; vi = vi->next) {
		if (!strcmp(name, vi->name)) {
			vi->value = value;
			return;
		}
	}
	len = strlen(name) + 1;
	vi = malloc(sizeof(*vi) + len);
	memcpy(vi->name, name, len);
	vi->value = value;
	vi->next = all_variables;
	all_variables = vi;
}

static int variable_get(const char *name, u32 *value) {
	struct varinfo *vi;
	for (vi = all_variables; vi; vi = vi->next) {
		if (!strcmp(name, vi->name)) {
			*value = vi->value;
			return 0;
		}
	}
	return -1;
}

int debugger_variable(const char *name, u32 *value) {
	return variable_get(name, value);
}

static int do_script(int argc, param *argv) {
	FILE *fp;
	char *line,linebuf[256];
	struct funcinfo *newfunc = 0;
	struct funcline *lastline = 0;
	const char *fname = argv[0].s;

	if (argc != 1)
		return -1;

	if (!strcmp(fname, "-")) {
		fp = stdin;
	} else {
		if (!(fp = fopen(argv[0].s, "r"))) {
			xprintf("error: cannot open '%s'\n", argv[0].s);
			return -1;
		}
	}

	while (fgets(linebuf, sizeof(linebuf), fp)) {
		line = linebuf;
		while (isspace(*line))
			line++;
		if (iscntrl(line[0]) || line[0] == '#')
			continue;
		if (!strncmp(line, "function", 8) && isspace(line[8])) {
			char *name, *x;
			name = line + 9;
			if (newfunc) {
				xprintf("error: nested functions not allowed\n");
				break;
			}
			while (isspace(*name))
				name++;
			x = name;
			while (!isspace(*x) && *x)
				x++;
			*x = 0;
			if (*name == 0) {
				xprintf("error: functions must have names\n");
				break;
			}
			newfunc = malloc(sizeof(*newfunc) + strlen(name) + 1);
			if (newfunc == 0) {
				xprintf("error: out of memory\n");
				break;
			}
			strcpy(newfunc->name, name);
			newfunc->next = 0;
			newfunc->lines = 0;
			continue;
		}
		if (newfunc) {
			struct funcline *fl;
			if (!strncmp(line, "end", 3) && isspace(line[3])) {
				newfunc->next = allfuncs;
				allfuncs = newfunc;
				newfunc = 0;
				lastline = 0;
				continue;
			}
			fl = malloc(sizeof(*fl) + strlen(line) + 1);
			if (fl == 0) {
				xprintf("out of memory");
				newfunc = 0;
				if (fp != stdin)
					fclose(fp);
				return -1;
			}
			strcpy(fl->text, line);
			fl->next = 0;
			if (lastline) {
				lastline->next = fl;
			} else {
				newfunc->lines = fl;
			}
			lastline = fl;
		} else {
			xprintf("script> %s", line);
			if (debugger_command(line))
				return -1;
		}
	}
	if (fp != stdin)
		fclose(fp);

	if (newfunc)
		newfunc = 0;
	return 0;
}

static int do_set(int argc, param *argv) {
	const char *name;
	if ((argc != 2) && (argc != 4)) {
		xprintf("error: set requires two or four arguments\n");
		return -1;
	}
	name = argv[0].s;
	if (*name == '$')
		name++;
	if (*name == 0) {
		xprintf("error: empty name?!\n");
		return -1;
	}
	if (!isalpha(*name)) {
		xprintf("error: variable name must begin with a letter\n");
		return -1;
	}

	if (argc == 4) {
		u32 a, b, n;
		const char *op;
		a = argv[1].n;
 		op = argv[2].s;
		b = argv[3].n;
		if (!strcmp(op,"+")) {
			n = a + b;
		} else if (!strcmp(op, "-")) {
			n = a - b;
		} else if (!strcmp(op, "<<")) {
			n = a << b;
		} else if (!strcmp(op, ">>")) {
			n = a >> b;
		} else if (!strcmp(op, "*")) {
			n = a * b;
		} else if (!strcmp(op, "/")) {
			if (b == 0) {
				n = 0;
			} else {
				n = a / b;
			}
		} else {
			xprintf("error: set <var> <a> <op> <b> requires op: + - * / << >>\n");
			return -1;
		}
		variable_set(name, n);
	} else {	
		variable_set(name, argv[1].n);
	}
	return 0;
}

static int parse_number(const char *in, unsigned *out) {
	u32 value;
	char text[64];
	char *obrack;
	int r;

	strncpy(text, in, sizeof(text));
	text[sizeof(text)-1] = 0;

	/* handle dereference forms */
	obrack = strchr(text, '[');
	if (obrack) {
		unsigned base, index;
		char *cbrack;
		*obrack++ = 0;
		cbrack = strchr(obrack, ']');
		if (!cbrack)
			return -1;
		*cbrack = 0;
		if (parse_number(text, &base))
			return -1;
		if (parse_number(obrack, &index))
			return -1;
		if (read_memory_word(base + index, &value))
			return -1;
		*out = value;
		return 0;	
	}

	/* handle local $[0..9] and global $... variables */
	if (text[0] == '$') {
		if (isdigit(text[1]) && (text[2] == 0)) {
			r = atoi(text + 1);
			if (r > 0) {
				if (r <= frame_argc) {
					*out = frame_argv[r - 1].n;
					return 0;
				}
			}
			xprintf("no local variable %s\n", text);
			*out = 0;
			return 0;
		}
		if (variable_get(text + 1, &value) == 0) {
			*out = value;
		} else {
			xprintf("undefined variable '%s'\n", text + 1);
			*out = 0;
		}
		return 0;
	}

	/* handle registers */
	r = read_register(text, &value);
	if (r != ERROR_UNKNOWN) {
		*out = value;
		return r;
	}

	/* otherwise decimal or hex constants */
	if (text[0] == '.') {
		*out = strtoul(text + 1, 0, 10);
	} else {
		*out = strtoul(text, 0, 16);
	}
	return 0;
}

static int exec_function(struct funcinfo *f, int argc, param *argv) {
	param *saved_argv;
	int saved_argc;
	struct funcline *line;
	char text[256];
	int r, n;

	saved_argv = frame_argv;
	saved_argc = frame_argc;
	frame_argv = argv;
	frame_argc = argc;

	for (line = f->lines, n = 1; line; line = line->next, n++) {
		strcpy(text, line->text);
		r = debugger_command(text);
		if (r) {
			xprintf("error: %s: line %d\n", f->name, n);
			goto done;
		}
	}
	r = 0;
done:
	frame_argc = saved_argc;
	frame_argv = saved_argv;
	return r;
}

static int _debugger_exec(const char *cmd, unsigned argc, param *argv) {
	struct funcinfo *f;
	struct debugger_command *c;

	/* core built-ins */
	if (!strcasecmp(cmd, "set"))
		return do_set(argc, argv);
	if (!strcasecmp(cmd, "script"))
		return do_script(argc, argv);

	for (c = debugger_commands; c->name; c++) {
		if (!strcasecmp(cmd, c->name)) {
			int n;
			debugger_lock();
			n = c->func(argc, argv);
			debugger_unlock();
			return n;
		}
	}
	for (f = allfuncs; f; f = f->next) {
		if (!strcasecmp(cmd, f->name)) {
			return exec_function(f, argc, argv);
		}
	}
	return ERROR_UNKNOWN;
}

int debugger_invoke(const char *cmd, unsigned argc, ...) {
	param arg[32];
	unsigned n;
	va_list ap;

	if (argc > 31)
		return -1;

	va_start(ap, argc);
	for (n = 0; n < argc; n++) {
		arg[n].s = "";
		arg[n].n = va_arg(ap, unsigned);
	}
	return _debugger_exec(cmd, argc, arg);
}

int debugger_command(char *line) {
	param arg[32];
	unsigned c, n = 0;
	int r;

	while ((c = *line)) {
		if (c <= ' ') {
			line++;
			continue;
		}
		arg[n].s = line;
		for (;;) {
			if (c == 0) {
				n++;
				break;
			} else if (c == '#') {
				*line = 0;
				break;
			} else if (c <= ' ') {
				*line++ = 0;
				n++;
				break;
			}
			c = *++line;
		}
	}

	if (n == 0)
		return 0;

	for (c = 0; c < n; c++) {
		if (parse_number(arg[c].s, &(arg[c].n))) {
			xprintf("error: bad number: %s\n", arg[c].s);
			return -1;
		}
	}

	r = _debugger_exec(arg[0].s, n - 1, arg + 1);
	if (r == ERROR_UNKNOWN) {
		xprintf("unknown command: %s\n", arg[0].s);
	}
	return r;
}

