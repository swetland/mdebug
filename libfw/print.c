/* print.c
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

#include <fw/lib.h>

static const char hex[16] = "0123456789ABCDEF";

char *printhex(char *tmp, unsigned c, unsigned n)
{
	tmp[c] = 0;
	while (c-- > 0) {
		tmp[c] = hex[n & 0x0F];
		n >>= 4;
	}

	return tmp;
}

char *print_unsigned(char *tmp, int len, unsigned n, int w)
{
	len--;
	tmp[len] = '\0';

	if (w < 0 && n == 0) {
		tmp[--len] = '0';
	} else {
		while ((len >= 0) && (n || (w > 0))) {
			tmp[--len] = hex[n % 10];
			n /= 10;
			w--;
		}
	}

	return &tmp[len];
}

char *print_signed(char *tmp, int len, signed n, int w)
{
	char *s;

	if (n >= 0)
		return print_unsigned(tmp, len, n, w);

	s = print_unsigned(tmp + 1, len - 1, n * -1, w - 1);
	s--;
	*s = '-';
	return s;
}

char *print_ufixed(char *tmp, int len, unsigned n)
{
	char * s;
	char * point;

	n >>= 2;

	/* XXX: does not account for overflow */
	n *= 15625;
	s = print_unsigned(tmp + 1, len - 1, (n + 50000) / 100000 % 10, 1);
	point = s - 1;
	s = print_unsigned(tmp, (s - tmp), n / 1000000, -1);
	*point = '.';

	return s;
}

char *print_fixed(char *tmp, int len, signed n)
{
	char * s;

	if (n >= 0)
		return print_ufixed(tmp, len, n);

	s = print_ufixed(tmp + 1, len - 1, n * -1);
	s--;
	*s = '-';

	return s;
}

void vprintx(void (*_putc)(unsigned), const char *fmt, va_list ap) {
	unsigned c;
	const char *s;
	char tmp[16];

	while ((c = *fmt++)) {
		if (c != '%') {
			_putc(c);
			continue;
		}

		switch((c = *fmt++)) {
		case 0:
			return;

		case 'x':
			s = printhex(tmp, 8, va_arg(ap, unsigned));
			break;

		case 'h':
			s = printhex(tmp, 4, va_arg(ap, unsigned));
			break;

		case 'b':
			s = printhex(tmp, 2, va_arg(ap, unsigned));
			break;

		case 'u':
			s = print_unsigned(tmp, sizeof(tmp), va_arg(ap, unsigned), -1);
			break;

		case 'd':
			s = print_signed(tmp, sizeof(tmp), va_arg(ap, signed), -1);
			break;

		case 'f':
			s = print_fixed(tmp, sizeof(tmp), va_arg(ap, signed));
			break;

		case 's':
			s = va_arg(ap, const char *);
			break;

		case 'c':
			c = va_arg(ap, unsigned); /* fall through */

		default:
			_putc(c); continue;
		}

		while (*s)
			_putc(*s++);
	}
}

