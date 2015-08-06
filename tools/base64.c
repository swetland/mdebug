/* base64.h
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
#include <stdint.h>
#include <unistd.h>
#include <string.h>

typedef uint8_t u8;
typedef uint32_t u32;

#define OP_MASK  0xC0
#define OP_BITS  0x00
#define OP_END   0x40
#define OP_SKIP  0x80
#define OP_BAD   0xC0

static u8 DTABLE[256] =
	"\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\x80\x80\xc0\xc0\x80\xc0\xc0"
	"\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0"
	"\x80\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\x3e\xc0\xc0\xc0\x3f"
	"\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\xc0\xc0\xc0\x40\xc0\xc0"
	"\xc0\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e"
	"\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\xc0\xc0\xc0\xc0\xc0"
	"\xc0\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23\x24\x25\x26\x27\x28"
	"\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\xc0\xc0\xc0\xc0\xc0"
	"\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0"
	"\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0"
	"\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0"
	"\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0"
	"\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0"
	"\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0"
	"\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0"
	"\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0";

static u8 ETABLE[64] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

int base64_decode(u8 *data, u32 len, u8 *out)
{
	u8 *start = out;
	u32 tmp = 0, pos = 0;

	while (len-- > 0) {
		u8 x = DTABLE[*data++];
		switch (x & OP_MASK) {
		case OP_BITS:
			tmp = (tmp << 6) | x;
			if (++pos == 4) {
				*out++ = tmp >> 16;
				*out++ = tmp >> 8;
				*out++ = tmp;
				tmp = 0;
				pos = 0;
			}
			break;
		case OP_SKIP:
			break;
		case OP_END:
			if (pos == 2) {
				if (*data != '=')
					return -1;
				*out++ = (tmp >> 4);
			} else if (pos == 3) {
				*out++ = (tmp >> 10);
				*out++ = (tmp >> 2);
			} else {
				return -1;
			}
			return out - start;
		case OP_BAD:
			return -1;
		}
	}

	if (pos != 0)
		return -1;

	return out - start;
}

int base64_encode(u8 *data, u32 len, u8 *out)
{
	u8 *start = out;
	u32 n;

	while (len >= 3) {
		n = (data[0] << 16) | (data[1] << 8) | data[2];
		data += 3;
		len -= 3;
		*out++ = ETABLE[(n >> 18) & 63];
		*out++ = ETABLE[(n >> 12) & 63];
		*out++ = ETABLE[(n >> 6) & 63];
		*out++ = ETABLE[n & 63];
	}
	if (len == 2) {
		*out++ = ETABLE[(data[0] >> 2) & 63];
		*out++ = ETABLE[((data[0] << 4) | (data[1] >> 4)) & 63];
		*out++ = ETABLE[(data[1] << 2) & 63];
		*out++ = '=';
	} else if (len == 1) {
		*out++ = ETABLE[(data[0] >> 2) & 63];
		*out++ = ETABLE[(data[0] << 4) & 63];
		*out++ = '=';
		*out++ = '=';
	}

	return out - start;
}
