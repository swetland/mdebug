/* swo.c
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
#include <unistd.h>
#include <string.h>

#include <pthread.h>
#include <fw/types.h>
#include <debugger.h>

void process_swo_data(void *_data, unsigned count) {
	unsigned char *data = _data;
	char buf[8192];
	char *p = buf;
	while (count-- > 0) {
		p += sprintf(p, "%02x ", *data++);
	}
	*p++ = '\n';
	*p++ = 0;
	xprintf(XDATA, buf);
}

