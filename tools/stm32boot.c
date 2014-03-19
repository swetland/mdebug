/* stm32boot.c
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
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <termios.h>
#include <signal.h>
#include <poll.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/types.h>

#if 0
int WRITE(int fd, void *ptr, int len) {
	int r;
	fprintf(stderr,"[");
	for (r = 0; r < len; r++) 
		fprintf(stderr," %02x", ((unsigned char *) ptr)[r]);
	fprintf(stderr," ]\n");
	return write(fd, ptr, len);
}

int READ(int fd, void *ptr, int len) {
	int n, r;
	r = read(fd, ptr, len);
	if (r <= 0) {
		fprintf(stderr,"<XX>\n");
		return r;
	}
	fprintf(stderr,"<");
	for (n = 0; n < len; n++) 
		fprintf(stderr," %02x", ((unsigned char *) ptr)[n]);
	fprintf(stderr," >\n");
	return r;
}

#define read READ
#define write WRITE
#endif

int openserial(const char *device, int speed)
{
	struct termios tio;
	int fd;
	fd = open(device, O_RDWR | O_NOCTTY);// | O_NDELAY);

	if (fd < 0)
		return -1;

	if (tcgetattr(fd, &tio))
		memset(&tio, 0, sizeof(tio));

	tio.c_cflag = B57600 | CS8 | CLOCAL | CREAD | PARENB;
	tio.c_ispeed = B57600;
	tio.c_ospeed = B57600;
	tio.c_iflag = IGNPAR;
	tio.c_oflag = 0;
	tio.c_lflag = 0; /* turn of CANON, ECHO*, etc */
	tio.c_cc[VTIME] = 1;
	tio.c_cc[VMIN] = 0;
	tcsetattr(fd, TCSANOW, &tio);
	tcflush(fd, TCIFLUSH);

#ifdef __APPLE__
	tio.c_cflag =  CS8 | CLOCAL | CREAD | PARENB;
#else
	tio.c_cflag =  speed | CS8 | CLOCAL | CREAD | PARENB;
#endif
	tio.c_ispeed = speed;
	tio.c_ospeed = speed;

	tcsetattr(fd, TCSANOW, &tio);
	tcflush(fd, TCIFLUSH);
	return fd;
}

#define ACK 0x79
#define NAK 0x1F

int readAck(int fd) { /* 3s timeout */
	int n;
	unsigned char x;
	for (n = 0; n < 30; n++) {
		if (read(fd, &x, 1) == 1) {
			if (x == ACK)
				return 0;
			fprintf(stderr,"?? %02x\n",x);	
			return -1;
		}
	}
	fprintf(stderr,"*TIMEOUT*\n");
	return -1;
}

int sendCommand(int fd, unsigned cmd) {
	unsigned char data[2];
	data[0] = cmd;
	data[1] = cmd ^ 0xFF;
	if (write(fd, data, 2) != 2)
		return -1;
	if (readAck(fd))
		return -1;
	return 0;
}

int sendAddress(int fd, unsigned addr) {
	unsigned char data[5];
	data[0] = addr >> 24;
	data[1] = addr >> 16;
	data[2] = addr >> 8;
	data[3] = addr;
	data[4] = data[0] ^ data[1] ^ data[2] ^ data[3];
	if (write(fd, data, 5) != 5)
		return -1;
	if (readAck(fd))
		return -1;
	return 0;
}

int sendLength(int fd, unsigned len) {
	unsigned char data[2];
	if ((len < 1) || (len > 256))
		return -1;
	len--;
	data[0] = len;
	data[1] = len ^ 0xFF;
	if (write(fd, data, 2) != 2)
		return -1;
	if (readAck(fd))
		return -1;
	return 0;
}

int sendData(int fd, void *ptr, unsigned len)
{
	unsigned char x;
	unsigned char check;
	unsigned char *data = ptr;
	unsigned n;

	if ((len < 1) || (len > 256))
		return -1;
	check = x = (len - 1);
	for (n = 0; n < len; n++) {
		check ^= data[n];
	}
	if (write(fd, &x, 1) != 1)
		return -1;
	if (write(fd, data, len) != len)
		return -1;
	if (write(fd, &check, 1) != 1)
		return -1;
	if (readAck(fd))
		return -1;
	return 0;	
}

int readData(int fd, void *ptr, int len)
{
	unsigned char *data = ptr;
	int r;
	while (len > 0) {
		r = read(fd, data, len);
		if (r <= 0) {
			fprintf(stderr,"*UNDERFLOW*\n");
			return -1;
		}
		len -= r;
		data += r;
	}
	return 0;
}

int readMemory(int fd, unsigned addr, void *ptr, int len)
{
	unsigned char *data = ptr;
	while (len > 0) {
		int xfer = (len > 256) ? 256 : len;
		fprintf(stderr,"read %04x at %08x -> %p\n", xfer, addr, data);
		if (sendCommand(fd, 0x11))
			return -1;
		if (sendAddress(fd, addr))
			return -1;
		if (sendLength(fd, xfer))
			return -1;
		if (readData(fd, data, xfer))
			return -1;
		data += xfer;
		len -= xfer;
		addr += xfer;
	}
	return 0;
}

int writeMemory(int fd, unsigned addr, void *ptr, int len)
{
	unsigned char *data = ptr;
	while (len > 0) {
		int xfer = (len > 256) ? 256 : len;
		if (sendCommand(fd, 0x31))
			return -1;
		if (sendAddress(fd, addr))
			return -1;
		if (sendData(fd, data, xfer))
			return -1;
		data += xfer;
		len -= xfer;
		addr += xfer;
	}
	return 0;
}

int eraseFlash(int fd)
{
	unsigned data[2] = { 0xFF, 0x00 };
	if (sendCommand(fd, 0x43))
		return -1;
	if (write(fd, data, 2) != 2)
		return -1;
	if (readAck(fd))
		return -1;
	return 0;
}

int jumpToAddress(int fd, unsigned addr)
{
	if (sendCommand(fd, 0x21))
		return -1;
	if (sendAddress(fd, addr))
		return -1;
	return 0;
}

int usage(void)
{
	fprintf(stderr,
		"usage: stm32boot [ erase | flash <file> | exec <file> ]\n");
	return -1;
}
int main(int argc, char *argv[])
{
	int speed = B115200;
	const char *device = "/dev/ttyUSB0";
	unsigned char x;
	int fd, n;
	unsigned char buf[32768];

	unsigned do_erase = 0;
	unsigned do_write = 0;
	unsigned do_exec = 0;
	unsigned addr = 0;

	if (argc < 2)
		return usage();

	if (!strcmp(argv[1],"erase")) {
		do_erase = 1;
	} else if (!strcmp(argv[1],"flash")) {
		do_erase = 1;
		do_write = 1;
		addr = 0x08000000;
	} else if (!strcmp(argv[1],"exec")) {
		do_write = 1;
		do_exec = 1;
		addr = 0x20001000;
	} else {
		return usage();
	}

	if (do_write && argc != 3)
		return usage();

	fd = openserial(device, speed);
	if (fd < 0) {
		fprintf(stderr, "stderr open '%s'\n", device);
		return -1;
	}

	n = TIOCM_DTR;
	ioctl(fd, TIOCMBIS, &n);
	usleep(2500);
	ioctl(fd, TIOCMBIC, &n);
	usleep(2500);

	/* If the board just powered up, we need to send an ACK
	 * to auto-baud and will get an ACK back.  If the board
	 * is already up, two ACKs will get a NAK (invalid cmd).
	 * Either way, we're talking!
	 */
	for (n = 0; n < 5; n++) {
		unsigned char SYNC = 0x7F;
		if (write(fd, &SYNC, 1)) { /* do nothing */ }
		if (read(fd, &x, 1) != 1)
			continue;
		if ((x == 0x79) || (x == 0x1f))
			break;
	}
	if (n == 5) {
		fprintf(stderr,"sync failure\n");
		return -1;
	}

#if 0
	readMemory(fd, 0x1FFFF000, buf, 4096);
	for (n = 0; n < 1024; n++) 
		fprintf(stderr,"%02x ", buf[n]);
	return 0;
#endif
	if (do_write) {
		int fd2 = open(argv[2], O_RDONLY);
		n = read(fd2, buf, sizeof(buf));
		close(fd2);
		if ((fd2 < 0) || (n <= 0)) {
			fprintf(stderr,"cannot read '%s'\n", argv[2]);
			return -1;
		}
		n += (n % 4);


		if (do_erase) {
			fprintf(stderr,"erasing flash...\n");
			if (eraseFlash(fd)) {
				fprintf(stderr,"erase failed\n");
				return -1;
			}
		}

		fprintf(stderr,"sending %d bytes...\n", n);
		if (writeMemory(fd, addr, buf, n)) {
			fprintf(stderr,"write failed\n");
			return -1;
		}
		fprintf(stderr,"done\n");

		if (do_exec) {
			jumpToAddress(fd, addr);
		} else {
			return 0;
		}
	} else if (do_erase) {
		if (eraseFlash(fd)) {
			fprintf(stderr,"erase failed\n");
			return -1;
		}
		fprintf(stderr,"flash erased\n");
		return 0;
	}
	
	for (;;) {
		if (read(fd, &x, 1) == 1) {
			if (x == 27) break;
			if ((x < 0x20) || (x > 0x7f))
				if ((x != 10) && (x != 13))
					x = '.';
			fprintf(stderr,"%c", x);
		}
	}
	
	return 0;
}
