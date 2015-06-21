/* socket.c
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

#include <unistd.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

int socket_listen_tcp(unsigned port) {
	int fd, n = 1;
	struct sockaddr_in addr;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(port);
	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		goto fail;
	}
	if (listen(fd, 10) < 0) {
		goto fail;
	}
	return fd;
fail:
	close(fd);
	return -1;
}
