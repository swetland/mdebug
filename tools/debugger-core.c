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
#include <errno.h>

#include <pthread.h>
#include <sys/socket.h>

#include <fw/types.h>
#include <protocol/rswdp.h>
#include "debugger.h"
#include "rswdp.h"

#include "websocket.h"

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

extern int swd_verbose;

#define GDB_SOCKET	5555
#define SWO_SOCKET	2332
#define WEB_SOCKET	5557

static void m_event(const char *evt) {
	xprintf(XCORE, "DEBUG EVENT: %s\n", evt);
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
static pthread_t _gdb_thread;

static pthread_mutex_t _swo_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t _swo_thread;
static pthread_t _ws_thread;
static int swo_fd = -1;
static ws_server_t *_ws = NULL;

void debugger_lock() {
	pthread_mutex_lock(&_dbg_lock);
	if (swdp_clear_error()) {
#if 0
		// way too noisy if the link goes down
		xprintf("SWD ERROR persists. Attempting link reset.\n");
#endif
		swdp_reset();
	}
	swd_verbose = 1;
}

void debugger_unlock() {
#if 0
	if (swdp_error()) {
		xprintf(XCORE, "SWD ERROR\n");
	}
#endif
	swd_verbose = 0;
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

void gdb_server(int fd);
int socket_listen_tcp(unsigned port);

void *gdb_listener(void *arg) {
	int fd;
	if ((fd = socket_listen_tcp(GDB_SOCKET)) < 0) {
		xprintf(XGDB, "gdb_listener() cannot bind to %d\n", GDB_SOCKET);
		return NULL;
	}
	for (;;) {
		int s = accept(fd, NULL, NULL);
		if (s >= 0) {
			gdb_server(s);
			close(s);
		}
	}
	return NULL;
}

void transmit_swo_data(void *data, unsigned len) {
	pthread_mutex_lock(&_swo_lock);
	if (swo_fd >= 0) {
		if (write(swo_fd, data, len)) ;
	}
	if (_ws != NULL) {
		unsigned char *x = data;
		x--;
		*x = 3; // SWO DATA MARKER
		ws_send_binary(_ws, x, len + 1);
	}
	pthread_mutex_unlock(&_swo_lock);
}

void *swo_listener(void *arg) {
	char buf[1024];
	int fd;
	if ((fd = socket_listen_tcp(SWO_SOCKET)) < 0) {
		xprintf(XCORE, "swo_listener() cannot bind to %d\n", SWO_SOCKET);
		return NULL;
	}
	for (;;) {
		int s = accept(fd, NULL, NULL);
		if (s >= 0) {
			xprintf(XCORE, "[ swo listener connected ]\n");
			pthread_mutex_lock(&_swo_lock);
			swo_fd = s;
			pthread_mutex_unlock(&_swo_lock);
			for (;;) {
				int r = read(s, buf, 1024);
				if (r < 0) {
					if (errno != EINTR) break;
				}
				if (r == 0) break;
			}
			pthread_mutex_lock(&_swo_lock);
			swo_fd = -1;
			pthread_mutex_unlock(&_swo_lock);
			close(s);
			xprintf(XCORE, "[ swo listener disconnected ]\n");
		}
	}
	return NULL;
}

static void ws_message(unsigned op, void *msg, size_t len, void *cookie) {
}

void *ws_listener(void *arg) {
	ws_server_t *ws;
	int fd;
	if ((fd = socket_listen_tcp(WEB_SOCKET)) < 0) {
		xprintf(XCORE, "websocket cannot bind to %d\n", WEB_SOCKET);
		return NULL;
	}
	for (;;) {
		int s = accept(fd, NULL, NULL);
		if (s >= 0) {
			ws = ws_handshake(s, ws_message, NULL);
			if (ws) {
				xprintf(XCORE, "[ websocket connected ]\n");
			} else {
				xprintf(XCORE, "[ websocket handshake failed ]\n");
				continue;
			}
			pthread_mutex_lock(&_swo_lock);
			_ws = ws;
			pthread_mutex_unlock(&_swo_lock);
			ws_process_messages(ws);
			pthread_mutex_lock(&_swo_lock);
			_ws = NULL;
			pthread_mutex_unlock(&_swo_lock);
			ws_close(ws);
			xprintf(XCORE, "[ websocket disconnected ]\n");
		}
	}
	return NULL;
}

void debugger_init() {
	pthread_create(&_dbg_thread, NULL, debugger_monitor, NULL);
	pthread_create(&_gdb_thread, NULL, gdb_listener, NULL);
	pthread_create(&_swo_thread, NULL, swo_listener, NULL);
	pthread_create(&_ws_thread, NULL, ws_listener, NULL);
}


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
			xprintf(XCORE, "error: cannot open '%s'\n", argv[0].s);
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
				xprintf(XCORE, "error: nested functions not allowed\n");
				break;
			}
			while (isspace(*name))
				name++;
			x = name;
			while (!isspace(*x) && *x)
				x++;
			*x = 0;
			if (*name == 0) {
				xprintf(XCORE, "error: functions must have names\n");
				break;
			}
			newfunc = malloc(sizeof(*newfunc) + strlen(name) + 1);
			if (newfunc == 0) {
				xprintf(XCORE, "error: out of memory\n");
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
				xprintf(XCORE, "out of memory");
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
			xprintf(XCORE, "script> %s", line);
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
		xprintf(XCORE, "error: set requires two or four arguments\n");
		return -1;
	}
	name = argv[0].s;
	if (*name == '$')
		name++;
	if (*name == 0) {
		xprintf(XCORE, "error: empty name?!\n");
		return -1;
	}
	if (!isalpha(*name)) {
		xprintf(XCORE, "error: variable name must begin with a letter\n");
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
			xprintf(XCORE, "error: set <var> <a> <op> <b> requires op: + - * / << >>\n");
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
			xprintf(XCORE, "no local variable %s\n", text);
			*out = 0;
			return 0;
		}
		if (variable_get(text + 1, &value) == 0) {
			*out = value;
		} else {
			xprintf(XCORE, "undefined variable '%s'\n", text + 1);
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
			xprintf(XCORE, "error: %s: line %d\n", f->name, n);
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

	while (*line && (*line == ' ')) line++;

	if (*line == '/') {
		arg[0].s = line + 1;
		return _debugger_exec("wconsole", 1, arg);
	}

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
			xprintf(XCORE, "error: bad number: %s\n", arg[c].s);
			return -1;
		}
	}

	r = _debugger_exec(arg[0].s, n - 1, arg + 1);
	if (r == ERROR_UNKNOWN) {
		xprintf(XCORE, "unknown command: %s\n", arg[0].s);
	}
	return r;
}

