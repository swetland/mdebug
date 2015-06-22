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

// useful gdb stuff
// set debug remote 1               protocol tracing
// set debug arch 1                 architecture tracing
// maint print registers-remote     check on register map

#define MAXPKT	8192

#define S_IDLE	0
#define S_RECV	1
#define S_CHK1	2
#define S_CHK2	3

#define F_ACK	1
#define F_TRACE	2

struct gdbcnxn {
	int fd;
	unsigned state;
	unsigned sum;
	unsigned flags;
	unsigned char *txptr;	
	unsigned char *rxptr;
	unsigned char rxbuf[MAXPKT];
	unsigned char txbuf[MAXPKT];
	char chk[4];
};

void zprintf(const char *fmt, ...) {
	linenoisePause();
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	linenoiseResume();
}

void gdb_init(struct gdbcnxn *gc, int fd) {
	gc->fd = fd;
	gc->state = S_IDLE;
	gc->sum = 0;
	gc->flags = F_ACK;
	gc->txptr = gc->txbuf;
	gc->rxptr = gc->rxbuf;
	gc->chk[2] = 0;
}

static inline int rx_full(struct gdbcnxn *gc) {
	return (gc->rxptr - gc->rxbuf) == (MAXPKT - 1);
}

static inline int tx_full(struct gdbcnxn *gc) {
	return (gc->txptr - gc->txbuf) == (MAXPKT - 1);
}

static inline void gdb_putc(struct gdbcnxn *gc, unsigned n) {
	unsigned char c = n;
	if (!tx_full(gc)) {
		gc->sum += c;
		*(gc->txptr++) = c;
	}
}

void gdb_puts(struct gdbcnxn *gc, const char *s) {
	while (*s) {
		gdb_putc(gc, *s++);
	}
}

void gdb_prologue(struct gdbcnxn *gc) {
	gc->txptr = gc->txbuf;
	*(gc->txptr++) = '$';
	gc->sum = 0;
}

void gdb_epilogue(struct gdbcnxn *gc) {
	unsigned char *ptr;
	int len, r;
	char tmp[4];
	sprintf(tmp,"#%02x", gc->sum & 0xff);
	gdb_puts(gc, tmp);

	if (tx_full(gc)) {
		zprintf("GDB: TX Packet Too Large\n");
		return;
	}

	ptr = gc->txbuf;
	len = gc->txptr - gc->txbuf;
	while (len > 0) {
		r = write(gc->fd, ptr, len);
		if (r <= 0) {
			if (errno == EINTR) continue;
			zprintf("GDB: TX Write Error\n");
			return;
		}
		ptr += r;
		len -= r;
	}
}

static char HEX[16] = "0123456789abcdef";
void gdb_puthex(struct gdbcnxn *gc, void *ptr, unsigned len) {
	unsigned char *data = ptr;
	while (len-- > 0) {
		unsigned c = *data++;
		gdb_putc(gc, HEX[c >> 4]);
		gdb_putc(gc, HEX[c & 15]);
	}
}

void handle_command(struct gdbcnxn *gc, unsigned char *cmd);

void gdb_recv_cmd(struct gdbcnxn *gc) {
	if (gc->flags & F_TRACE) {
		zprintf("PKT: %s\n", gc->rxbuf);
	}
	debugger_lock();
	handle_command(gc, gc->rxbuf);
	debugger_unlock();
}

int gdb_recv(struct gdbcnxn *gc, unsigned char *ptr, int len) {
	unsigned char *start = ptr;
	unsigned char c;

	while (len > 0) {
		c = *ptr++;
		len--;
		switch (gc->state) {
		case S_IDLE:
			if (c == 3) {
				gc->rxbuf[0] = 's';
				gc->rxbuf[1] = 0;
				gdb_recv_cmd(gc);
			} else if (c == '$') {
				gc->state = S_RECV;
				gc->sum = 0;
				gc->rxptr = gc->rxbuf;
			}
			break;
		case S_RECV:
			if (c == '#') {
				gc->state = S_CHK1;
			} else {
				if (rx_full(gc)) {
					zprintf("PKT: Too Large, Discarding.");
					gc->rxptr = gc->rxbuf;
					gc->state = S_IDLE;
				} else {
					*(gc->rxptr++) = c;
					gc->sum += c;
				}
			}
			break;
		case S_CHK1:
			gc->chk[0] = c;
			gc->state = S_CHK2;
			break;
		case S_CHK2:
			gc->chk[1] = c;
			gc->state = S_IDLE;
			*(gc->rxptr++) = 0;
			if (strtoul(gc->chk, NULL, 16) == (gc->sum & 0xFF)) {
				if (gc->flags & F_ACK) {
					if (write(gc->fd, "+", 1)) ;
				}
				gdb_recv_cmd(gc);
			} else {
				if (gc->flags & F_ACK) {
					if (write(gc->fd, "-", 1)) ;
				}
			}
			break;
		}
	}
	return ptr - start;
}

unsigned unhex(char *x) {
	unsigned n;
	char t = x[2];
	x[2] = 0;
	n = strtoul(x, 0, 16);
	x[2] = t;
 	return n;
}

static const char *target_xml =
"<?xml version=\"1.0\"?>"
"<target>"
"<architecture>arm</architecture>"
"<feature name=\"org.gnu.gdb.arm.m-profile\">"
"<reg name=\"r0\" bitsize=\"32\"/>"
"<reg name=\"r1\" bitsize=\"32\"/>"
"<reg name=\"r2\" bitsize=\"32\"/>"
"<reg name=\"r3\" bitsize=\"32\"/>"
"<reg name=\"r4\" bitsize=\"32\"/>"
"<reg name=\"r5\" bitsize=\"32\"/>"
"<reg name=\"r6\" bitsize=\"32\"/>"
"<reg name=\"r7\" bitsize=\"32\"/>"
"<reg name=\"r8\" bitsize=\"32\"/>"
"<reg name=\"r9\" bitsize=\"32\"/>"
"<reg name=\"r10\" bitsize=\"32\"/>"
"<reg name=\"r11\" bitsize=\"32\"/>"
"<reg name=\"r12\" bitsize=\"32\"/>"
"<reg name=\"sp\" bitsize=\"32\" type=\"data_ptr\"/>"
"<reg name=\"lr\" bitsize=\"32\"/>"
"<reg name=\"pc\" bitsize=\"32\" type=\"code_ptr\"/>"
"<reg name=\"xpsr\" bitsize=\"32\"/>"
"<reg name=\"msp\" bitsize=\"32\" type=\"data_ptr\"/>"
"<reg name=\"psp\" bitsize=\"32\" type=\"data_ptr\"/>"
"</feature>"
"</target>";

static void handle_query(struct gdbcnxn *gc, char *cmd, char *args) {
	if (!strcmp(cmd, "fThreadInfo")) {
		/* report just one thread id, #1, for now */
		gdb_puts(gc, "m1");
	} else if(!strcmp(cmd, "sThreadInfo")) {
		/* no additional thread ids */
		gdb_puts(gc, "l");
	} else if(!strcmp(cmd, "ThreadExtraInfo")) {
		/* gdb manual suggest 'Runnable', 'Blocked on Mutex', etc */
		/* informational text shown in gdb's "info threads" listing */
		gdb_puthex(gc, "Native", 6);
	} else if(!strcmp(cmd, "C")) {
		/* current thread ID */
		gdb_puts(gc, "QC1");
	} else if (!strcmp(cmd, "Rcmd")) {
		char *p = args;
		cmd = p;
		while (p[0] && p[1]) {
			*cmd++ = unhex(p);
			p+=2;
		}
		*cmd = 0;
		debugger_command(args);
	} else if(!strcmp(cmd, "Supported")) {
		gdb_puts(gc,
			"qXfer:features:read+"
			";QStartNoAckMode+"
			";PacketSize=2004" /* size includes "$" and "#xx" */
			);
	} else if(!strcmp(cmd, "Xfer")) {
		if (!strncmp(args, "features:read:target.xml:", 25)) {
			gdb_puts(gc,"l");
			// todo: support binary format w/ escaping
			gdb_puts(gc, target_xml);
		}
	} else if(!strcmp(cmd, "TStatus")) {
		/* tracepoints unsupported. ignore. */
	} else if(!strcmp(cmd, "Attached")) {
		/* no process management. ignore */
	} else {
		zprintf("GDB: unsupported: q%s:%s\n", cmd, args);
	}
}

static void handle_set(struct gdbcnxn *gc, char *cmd, char *args) {
	if(!strcmp(cmd, "StartNoAckMode")) {
		gc->flags &= ~F_ACK;
		gdb_puts(gc, "OK");
	} else {
		zprintf("GDB: unsupported: Q%s:%s\n", cmd, args);
	}
}

void handle_command(struct gdbcnxn *gc, unsigned char *cmd) {
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
		if (sscanf((char*) cmd + 1, "%x,%x", &x, &n) != 2)
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
	case 'p': {
		u32 v;
		swdp_core_read(strtoul((char*) cmd + 1, NULL, 16), &v);
		gdb_puthex(gc, &v, sizeof(v));
		break;
	}
	case 's':
		swdp_core_step();
		gdb_puts(gc, "S00");
		break;
	case 'q': 
	case 'Q': {
		char *args = (char*) (cmd + 1);
		while (*args) {
			if ((*args == ':') || (*args == ',')) {
				*args++ = 0;
				break;
			}
			args++;
		}
		if (cmd[0] == 'q') {
			handle_query(gc, (char*) (cmd + 1), args);
		} else {
			handle_set(gc, (char*) (cmd + 1), args);
		}
		break;
		
	}
	default:
		zprintf("GDB: unknown command: %c\n", cmd[0]);
	}
	gdb_epilogue(gc);
}

void gdb_server(int fd) {
	struct gdbcnxn gc;
	unsigned char iobuf[32768];
	unsigned char *ptr;
	int r, len;

	gdb_init(&gc, fd);

	gc.flags |= F_TRACE;

	for (;;) {
		r = read(fd, iobuf, sizeof(iobuf));
		if (r <= 0) {
			if (errno == EINTR) continue;
			return;
		}
		len = r;
		ptr = iobuf;
		while (len > 0) {
			r = gdb_recv(&gc, ptr, len);
			ptr += r;
			len -= r;
		}
	}
}
