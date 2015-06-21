/* gdb-bridge.c
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
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <fw/types.h>
#include "rswdp.h"
#include <protocol/rswdp.h>
#include "debugger.h"
#include "linenoise.h"

void zprintf(const char *fmt, ...) {
	linenoisePause();
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	linenoiseResume();
}

struct gdbcnxn {
	int tx, rx;
	unsigned chk;
};

int gdb_getc(struct gdbcnxn *gc) {
	int r;
	unsigned char c;
	for (;;) {
		r = read(gc->rx, &c, 1);
		if (r <= 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		return c;
	}
}

int gdb_putc(struct gdbcnxn *gc, unsigned n) {
	unsigned char c = n;
	int r;
	gc->chk += c;
	for (;;) {
		r = write(gc->tx, &c, 1);
		if (r <= 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		return 0;
	}
}
int gdb_puts(struct gdbcnxn *gc, const char *s) {
	int r;
	while (*s) {
		r = gdb_putc(gc, *s++);
		if (r < 0) return r;
	}
	return 0;
}
int gdb_prologue(struct gdbcnxn *gc) {
	int n = gdb_putc(gc, '$');
	gc->chk = 0;
	return n;
}
int gdb_epilogue(struct gdbcnxn *gc) {
	char tmp[4];
	sprintf(tmp,"#%02x", gc->chk & 0xff);
	return gdb_puts(gc, tmp);
}

static char HEX[16] = "0123456789abcdef";
int gdb_puthex(struct gdbcnxn *gc, void *ptr, unsigned len) {
	unsigned char *data = ptr;
	unsigned n;
	char buf[1025];

	n = 0;
	while (len-- > 0) {
		unsigned c = *data++;
		buf[n++] = HEX[c >> 4];
		buf[n++] = HEX[c & 15];
		if (n == 1024) {
			buf[n] = 0;
			if (gdb_puts(gc, buf))
				return -1;
			n = 0;
		}
	}
	if (n) {
		buf[n] = 0;
		return gdb_puts(gc, buf);
	}
	return 0;
}

int gdb_recv(struct gdbcnxn *gc, char *buf, int max) {
	char *start = buf;
	unsigned chk;
	char tmp[3];
	int c;

again:
	do {
		c = gdb_getc(gc);
		if (c < 0) goto fail;
		if (c == 3) {
			buf[0] = 's';
			buf[1] = 0;
			return 0;
		}
		if (c < 0x20)
			zprintf("PKT: ?? %02x\n",c);
	} while (c != '$');

	chk = 0;
	while (max > 1) {
		c = gdb_getc(gc);
		if (c == '#') {
			*buf++ = 0;
			c = gdb_getc(gc);
			if (c < 0) goto fail;
			tmp[0] = c;
			c = gdb_getc(gc);
			if (c < 0) goto fail;
			tmp[1] = c;
			c = strtoul(tmp, 0, 16);
			if (c != (chk & 0xff)) {
				gdb_putc(gc,'-');
				zprintf("PKT: BAD CHECKSUM\n");
				goto again;
			} else {
				gdb_putc(gc,'+');
				return 0;
			}
		} else {
			chk += c;
			*buf++ = c;
		}
	}
	gdb_putc(gc,'-');
	zprintf("PKT: OVERFLOW\n");
	goto again;

fail:
	*start = 0;
	return -1;
}

unsigned unhex(char *x) {
	unsigned n;
	char t = x[2];
	x[2] = 0;
	n = strtoul(x, 0, 16);
	x[2] = t;
 	return n;
}

static struct gdbcnxn *GC;

#if 0
void xprintf(const char *fmt, ...) {
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	buf[sizeof(buf)-1] = 0;
	va_end(ap);
	gdb_puthex(GC, buf, strlen(buf));
}
#endif

void handle_ext_command(struct gdbcnxn *gc, char *cmd, char *args) {
	if (!strcmp(cmd,"Rcmd")) {
		char *p = args;
		cmd = p;
		while (p[0] && p[1]) {
			*cmd++ = unhex(p);
			p+=2;
		}
		*cmd = 0;
		GC = gc;
		debugger_command(args);
	}
}

void handle_command(struct gdbcnxn *gc, char *cmd) {
	union {
		u32 w[256+2];
		u16 h[512+4];
		u8 b[1024+8];
	} tmp;
	unsigned n,x;

	/* silent (no-response) commands */
	switch (cmd[0]) {
	case 'c':
		swdp_core_resume();
		return;
	}

	gdb_prologue(gc);
	switch (cmd[0]) {
	case '?':
		gdb_puts(gc, "S00");
		swdp_core_halt();
		break;
	case 'H':
		gdb_puts(gc, "OK");
		break;
	case 'm':
		if (sscanf(cmd + 1, "%x,%x", &x, &n) != 2)
			break;

		if (n > 1024)
			n = 1024;
		 
		swdp_ahb_read32(x & (~3), tmp.w, ((n + 3) & (~3)) / 4);
		gdb_puthex(gc, tmp.b + (x & 3), n);
		break;	
	case 'g':  {
		u32 regs[19];
		swdp_core_read_all(regs);
		gdb_puthex(gc, regs, sizeof(regs));
		break;
	}
	case 's':
		swdp_core_step();
		gdb_puts(gc, "S00");
		break;
	case 'q': {
		char *args = ++cmd;
		while (*args) {
			if (*args == ',') {
				*args++ = 0;
				break;
			}
			args++;
		}
		handle_ext_command(gc, cmd, args);
		break;
		
	}
	default:
		zprintf("CMD: %c unknown\n", cmd[0]);
	}
	gdb_epilogue(gc);
}

void handler(int n) {
}

void gdb_server(int fd) {
	struct gdbcnxn gc;
	char cmd[32768];
	gc.tx = fd;
	gc.rx = fd;
	gc.chk = 0;

	while (gdb_recv(&gc, cmd, sizeof(cmd)) == 0) {
		zprintf("PKT: %s\n", cmd);
		debugger_lock();
		handle_command(&gc, cmd);
		debugger_unlock();
	}
}
