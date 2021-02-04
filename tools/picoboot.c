// Copyright 2020, Brian Swetland <swetland@frotz.net>
// Licensed under the Apache License, Version 2.0.

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>

#define ERASE_MASK (4096 - 1)
#define BLOCK_MASK (256 - 1)

void *load_file(const char *fn, size_t *_sz) {
        int fd;
        off_t sz;
        void *data = NULL;
        fd = open(fn, O_RDONLY);
        if (fd < 0) goto fail;
        sz = lseek(fd, 0, SEEK_END);
        if (sz < 0) goto fail;
        if (lseek(fd, 0, SEEK_SET)) goto fail;
        if ((data = malloc(sz)) == NULL) goto fail;
        if (read(fd, data, sz) != sz) goto fail;
        *_sz = sz;
        return data;
fail:
        if (data) free(data);
        if (fd >= 0) close(fd);
        return NULL;
}

#define PB_VID 0x2e8a
#define PB_PID 0x0003

#define PB_IFC 1
#define PB_EP_IN 0x84
#define PB_EP_OUT 0x03

#define PB_MAGIC 0x431fd10b

typedef struct {
	uint32_t magic;
	uint32_t token;
	uint8_t cmdid;
	uint8_t argslen;
	uint16_t reserved;
	uint32_t xferlen;
	union {
		uint8_t u8[16];
		uint32_t u32[4];
	};
} pbcmd_t;

typedef struct {
	uint32_t token;
	uint32_t status;
	uint8_t cmd;
	uint8_t busy;
	uint8_t reserved[6];
} pbstatus_t;

#define CMD_EXCLUSIVE_ACCESS 0x01 // excl:u8
#define CMD_REBOOT           0x02 // pc:u32 sp:u32 delay:u32
#define CMD_FLASH_ERASE      0x03 // addr:u32 size:u32
#define CMD_READ             0x04 // addr:u32 size:u32
#define CMD_WRITE            0x05 // addr:u32 size:u32
#define CMD_EXIT_XIP         0x06
#define CMD_ENTER_XIP        0x07
#define CMD_EXEC             0x08 // addr:u32
#define CMD_VECTORIZE_FLASH  0x09 // addr:u32

#define EA_NOT 0
#define EA_EXCLUSIVE 1
#define EA_EXCLUSIVE_AND_EJECT 2

static libusb_context* usbctx = NULL;
static libusb_device_handle* usbdev = NULL;
static uint32_t token = 0;

int pb_status(void) {
	pbstatus_t pbs;
	int r = libusb_control_transfer(usbdev,
			LIBUSB_REQUEST_TYPE_VENDOR |
			LIBUSB_RECIPIENT_INTERFACE |
			LIBUSB_ENDPOINT_IN,
			0x42, 0, 1, (void*) &pbs, sizeof(pbs), 1000);
	if (r != sizeof(pbs)) {
		fprintf(stderr, "picoboot: cannot read status %d\n", r);
		return -1;
	}
	fprintf(stderr, "picoboot: status: tok=%08x sts=%08x cmd=%02x busy=%02x\n",
		pbs.token, pbs.status, pbs.cmd, pbs.busy);
	return pbs.status;
}

int TRACE = 0;

int pb_txn(pbcmd_t* cmd, void* data, int tx) {
	cmd->magic = PB_MAGIC;
	cmd->token = token++;
	int xfer;
	int r;
	uint8_t tmp[64];

	if (TRACE) fprintf(stderr, "picoboot: txn cmd=%02x len=%u\n", cmd->cmdid, cmd->xferlen);
	if ((r = libusb_bulk_transfer(usbdev, PB_EP_OUT, (void*) cmd, sizeof(pbcmd_t), &xfer, 0)) < 0) {
		fprintf(stderr, "picoboot: usb error sending command %d\n", r);
		pb_status();
		return r;
	}
	if (TRACE) pb_status();

	if (tx && (cmd->xferlen > 0)) {
		xfer = 0;
		if ((r = libusb_bulk_transfer(usbdev, PB_EP_OUT, data, cmd->xferlen, &xfer, 5000)) < 0) {
			fprintf(stderr, "xfer %u\n", xfer);
			fprintf(stderr, "picoboot: usb error sending payload %d\n", r);
			pb_status();
			return r;
		}
	}

	xfer = 0;
	if ((r = libusb_bulk_transfer(usbdev, PB_EP_IN, tmp, 64, &xfer, 0)) < 0) {
		fprintf(stderr, "xfer %u\n", xfer);
		fprintf(stderr, "picoboot: usb error receiving ack %d\n", r);
		pb_status();
		return -1;
	}

	if (TRACE) pb_status();
	return 0;
}

void usage(void) {
	fprintf(stderr,
		"usage: picoboot ( cmd )*\n"
		"\n"
		"commands: -flash <filename> ( @hex ) write file to flash\n"
		"          -reboot                    reboot device\n"
		"          -help                      show this\n"
		);
}

int pb_io(uint8_t _cmd, uint32_t addr, uint32_t size, void* data) {
	pbcmd_t cmd;
	memset(&cmd, 0, sizeof(cmd));
	cmd.cmdid = _cmd;
	cmd.argslen = 8;
	cmd.u32[0] = addr;
	cmd.u32[1] = size;
	if (data) cmd.xferlen = size;
	return pb_txn(&cmd, data, 1);
}

int pb_cmd(uint8_t _cmd) {
	pbcmd_t cmd;
	memset(&cmd, 0, sizeof(cmd));
	cmd.cmdid = _cmd;
	switch (_cmd) {
	case CMD_EXCLUSIVE_ACCESS:
		cmd.argslen = 1;
		cmd.u8[0] = EA_EXCLUSIVE;
		break;
	case CMD_REBOOT:
		cmd.argslen = 12;
		cmd.u32[2] = 100; // ms
		break;
	}
	return pb_txn(&cmd, NULL, 0);
}

int main(int argc, char** argv) {
	if (argc == 1) {
		usage();
		return 0;
	}

	if (libusb_init(&usbctx) < 0) return -1;
	if ((usbdev = libusb_open_device_with_vid_pid(usbctx, PB_VID, PB_PID)) == NULL) {
		fprintf(stderr, "picoboot: cannot find compatible device\n");
		return -1;
	}
	if ((libusb_claim_interface(usbdev, PB_IFC)) < 0) {
		fprintf(stderr, "picoboot: cannot claim interface\n");
		return -1;
	}
	libusb_clear_halt(usbdev, PB_EP_IN);
	libusb_clear_halt(usbdev, PB_EP_OUT);

	size_t sz;
	void* data;
	for (argc--, argv++; argc > 0; argc--, argv++) {
		if (!strcmp(argv[0], "-reboot")) {
			if (pb_cmd(CMD_REBOOT)) {
				fprintf(stderr, "picoboot: REBOOT failed\n");
				return -1;
			}
		} else if (!strcmp(argv[0], "-flash")) {
			const char *fn = argv[1];
			if (argc < 2) {
				fprintf(stderr, "picoboot: missing argument\n");
				return -1;
			}
			if ((data = load_file(fn, &sz)) == NULL) {
				fprintf(stderr, "picoboot: failed to load '%s'\n", fn);
				return -1;
			}
			argc--; argv++;
			uint32_t addr = 0x10000000;
			if ((argc > 1) && (argv[1][0] == '@')) {
				addr = strtoul(argv[1] + 1, NULL, 16);
				argc--; argv++;
			}
			if ((addr < 0x10000000) || (addr >= 0x11000000) || (addr & ERASE_MASK)) {
				fprintf(stderr, "picoboot: bad flash start address 0x%08x\n", addr);
				return -1;
			}
			if (pb_cmd(CMD_EXCLUSIVE_ACCESS)) {
				fprintf(stderr, "picoboot: EXCLUSIVE ACCESS failed\n");
				return -1;
			}
			if (pb_cmd(CMD_EXIT_XIP)) {
				fprintf(stderr, "picoboot: EXIT XIP failed\n");
				return -1;
			}
			fprintf(stderr, "picoboot: erase flash @ 0x%08x\n", addr);
			if (pb_io(CMD_FLASH_ERASE, addr, (sz + ERASE_MASK) & (~ERASE_MASK), NULL)) {
				fprintf(stderr, "picoboot: ERASE failed\n");
				return -1;
			}
			fprintf(stderr, "picoboot: write '%s' to flash @ 0x%08x\n", fn, addr);
			if (pb_io(CMD_WRITE, addr, sz, data)) {
				fprintf(stderr, "picoboot: WRITE failed\n");
				return -1;
			}
		} else if (!strcmp(argv[0], "-help")) {
			usage();
			return 0;
		} else {
			fprintf(stderr, "picoboot: unknown command '%s'\n", argv[0]);
			usage();
			return -1;
		}
	}

	return 0;
}
