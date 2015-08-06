/* websocket.c
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
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include "sha1.h"
#include "websocket.h"

struct ws_server {
	FILE *fp;
	int fd;
	ws_callback_t func;
	void *cookie;
};

int base64_decode(uint8_t *data, uint32_t len, uint8_t *out);
int base64_encode(uint8_t *data, uint32_t len, uint8_t *out);

static const char *wsmagic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

#define F_FIN		0x80
#define F_INVAL		0x70
#define OP_NONE		0
#define OP_TEXT		1
#define OP_BINARY	2
#define OP_CLOSE	8
#define OP_PING		9
#define OP_PONG		10

#define F_MASKED	0x80
#define LEN_16BIT	0x7E
#define LEN_64BIT	0x7F

static int ws_send(int fd, unsigned op, const void *data, size_t len);

int ws_send_text(ws_server_t *ws, const void *data, size_t len) {
	return ws_send(ws->fd, OP_TEXT, data, len);
}

int ws_send_binary(ws_server_t *ws, const void *data, size_t len) {
	return ws_send(ws->fd, OP_BINARY, data, len);
}

static int _ws_handshake(ws_server_t *ws) {
	FILE *fp = ws->fp;
	unsigned check = 0;
	char line[8192 + 128];
	uint8_t digest[SHA_DIGEST_SIZE];
	int n;

	for (;;) {
		if (fgets(line, sizeof(line), fp) == NULL) {
			return -1;
		}
		if ((n = strlen(line)) == 0) {
			return -1;
		}
		if (line[n-1] == '\n') {
			line[n-1] = 0;
			if ((n > 1) && (line[n-2] == '\r')) {
				line[n-2] = 0;
			}
		}
		if (line[0] == 0) {
			//printf(">BREAK<\n");
			break;
		}
		//printf(">HEADER: %s<\n", line);
		if (!strcasecmp(line, "upgrade: websocket")) {
			check |= 1;
			continue;
		}
		if (!strcasecmp(line, "connection: upgrade")) {
			// may be keep-alive, etc
			check |= 2;
			continue;
		}
		if (!strncasecmp(line, "sec-websocket-key: ", 19)) {
			check |= 4;
			strcat(line, wsmagic);
			SHA(line + 19, strlen(line + 19), digest);
		}
		if (!strcasecmp(line, "sec-websocket-version: 13")) {
			check |= 8;
		}
		// Host:
		// Origin:
		// Sec-WebSocket-Protocol:
		// Sec-WebSocket-Extensions:
	}
	if ((check & 13) != 13) {
		return -1;
	}
	n = base64_encode((void*) digest, SHA_DIGEST_SIZE, (void*) line);
	line[n] = 0;
	fprintf(fp,
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-Websocket-Accept: %s\r\n"
		"\r\n", line
	);
	return 0;
}

int ws_process_messages(ws_server_t *ws) {
	FILE *fp = ws->fp;
	unsigned char hdr[2+2+4];
	unsigned char msg[65536];
	unsigned len;
	for (;;) {
		if (fread(hdr, 2, 1, fp) != 1) break;
		// client must not set invalid flags
		if (hdr[0] & F_INVAL) break;
		// client must mask
		if (!(hdr[1] & F_MASKED)) break;
		// todo: fragments
		if (!(hdr[0] & F_FIN)) break;
		// todo: large packets
		if ((hdr[1] & 0x7F) == LEN_64BIT) break;
		if ((hdr[1] & 0x7F) == LEN_16BIT) {
			if (fread(hdr + 2, 6, 1, fp) != 1) break;
			len = (hdr[2] << 8) | hdr[3];
		} else {
			if (fread(hdr + 4, 4, 1, fp) != 1) break;
			len = hdr[1] & 0x7F;
		}
#if 0
		printf("<op %d len %d mask %02x%02x%02x%02x>\n",
			hdr[0] & 15, len, hdr[4], hdr[5], hdr[6], hdr[7]);
#endif
		if (fread(msg, len, 1, fp) != 1) break;
		// op?
#if 0
		unsigned i;
		for (i = 0; i < len; i++) {
			msg[i] ^= hdr[4 + (i & 3)];
			printf("%02x ", msg[i]);
		}
		printf("\n");
#endif
		switch (hdr[0] & 15) {
		case OP_PING:
			break;
		case OP_PONG:
			break;
		case OP_CLOSE:
			// todo: reply
			return 0;
		case OP_TEXT:
			ws->func(OP_TEXT, msg, len, ws->cookie);
			break;
		case OP_BINARY:
			ws->func(OP_BINARY, msg, len, ws->cookie);
			break;
		default:
			// invalid opcode
			return 0;
		}
	}
	return 0;
}

static int ws_send(int fd, unsigned op, const void *data, size_t len) {
	unsigned char hdr[4];
	struct iovec iov[2];
	if (len > 65535) return -1;
	hdr[0] = F_FIN | (op & 15);
	if (len < 126) {
		hdr[1] = len;
		iov[0].iov_len = 2;
	} else {
		hdr[1] = LEN_16BIT;
		hdr[2] = len >> 8;
		hdr[3] = len;
		iov[0].iov_len = 4;
	}
	iov[0].iov_base = hdr;
	iov[1].iov_base = (void*) data;
	iov[1].iov_len = len;
	return writev(fd, iov, 2);
}

ws_server_t *ws_handshake(int fd, ws_callback_t func, void *cookie) {
	ws_server_t *ws;

	if ((ws = malloc(sizeof(ws_server_t))) == NULL) {
		goto fail;
	}
	if ((ws->fp = fdopen(fd, "r+")) == NULL) {
		goto fail;
	}
	if (_ws_handshake(ws)) {
		goto fail;
	}
	ws->fd = fd;
	ws->func = func;
	ws->cookie = cookie;
	return ws;
fail:
	if (ws != NULL) {
		free(ws);
		if (ws->fp) {
			fclose(ws->fp);
			return NULL;
		}
	}
	close(fd);
	return NULL;
}

void ws_close(ws_server_t *ws) {
	fclose(ws->fp);
	free(ws);
}
