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
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <getopt.h>

#include "linenoise.h"

#include <fw/types.h>
#include "rswdp.h"

static const char *scriptfile = NULL;

void xprintf(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void debugger_command(char *line);

static void handler(int n) {
	swdp_interrupt();
}

static void usage(int argc, char **argv) {
	fprintf(stderr, "usage: %s [-h] [-f script]\n", argv[0]);

	exit(1);
}

int main(int argc, char **argv) {
	char lastline[1024];
	char *line;

	/* args */
	for(;;) {
		int c;
		int option_index = 0;

		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			{"script", 1, 0, 'f'},
			{0, 0, 0, 0},
		};

		c = getopt_long(argc, argv, "f:h", long_options, &option_index);
		if(c == -1)
			break;

		switch(c) {
			case 'f':
				scriptfile = optarg;
				break;
			case 'h':
				usage(argc, argv);
				break;
			default:
				usage(argc, argv);
				break;
		}
	}

	lastline[0] = 0;

	if (swdp_open()) {
		fprintf(stderr,"could not find device\n");
		return -1;
	}

	signal(SIGINT, handler);

//	swdp_enable_tracing(1);

	/*
	 * if the user passed in a script file, pass it to the debug
	 * command handler before starting the main loop
	 */
	if (scriptfile != NULL) {
		char *buf = malloc(sizeof("script ") + strlen(scriptfile) + 1);

		strcpy(buf, "script ");
		strcat(buf, scriptfile);

		debugger_command(buf);

		free(buf);
	}

	while ((line = linenoise("debugger> ")) != NULL) {
		if (line[0] == 0) {
			strcpy(line, lastline);
		} else {
			linenoiseHistoryAdd(line);
			strcpy(lastline, line);
		}
		debugger_command(line);
	}
	return 0;
}
