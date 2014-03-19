/* debugger.c
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
#include <stdint.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
	int fd;
	uint32_t tmp, chk, n;

	chk = 0;

	if (argc != 2)
		return -1;
	if ((fd = open(argv[1], O_RDWR)) < 0)
		return -1;
	for (n = 0; n < 7; n++) {
		if (read(fd, &tmp, 4) != 4)
			return -1;
		chk += tmp;
	}
	tmp = - chk;
	n = write(fd, &tmp, 4);
	close(fd);
	return 0;
}
