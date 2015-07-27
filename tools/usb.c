/* usb.c
 *
 * Copyright 2014 Brian Swetland <swetland@frotz.net>
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

#include <stdlib.h>
#include <stdio.h>

#include <libusb-1.0/libusb.h>

#include "usb.h"

struct usb_handle {
	libusb_device_handle *dev;
	unsigned ei;
	unsigned eo;
};

static libusb_context *usb_ctx = NULL;

usb_handle *usb_open(unsigned vid, unsigned pid, unsigned ifc) {
	usb_handle *usb;
	int r;

	if (usb_ctx == NULL) {
		if (libusb_init(&usb_ctx) < 0) {
			usb_ctx = NULL;
			return NULL;
		}
	}

	usb = malloc(sizeof(usb_handle));
	if (usb == 0) {
		return NULL;
	}

	/* TODO: extract from descriptors */
	switch (ifc) {
	case 0:
		usb->ei = 0x81;
		usb->eo = 0x01;
		break;
	case 1:
		usb->ei = 0x82;
		usb->eo = 0x02;
		break;
	default:
		goto fail;
	}
		
	usb->dev = libusb_open_device_with_vid_pid(usb_ctx, vid, pid);
	if (usb->dev == NULL) {
		goto fail;
	}
	// This causes problems on re-attach.  Maybe need for OSX?
	// On Linux it's completely happy without us explicitly setting a configuration.
	//r = libusb_set_configuration(usb->dev, 1);
	r = libusb_claim_interface(usb->dev, ifc);
	if (r < 0) {
		fprintf(stderr, "failed to claim interface #%d\n", ifc);
		goto close_fail;
	}
	
	return usb;

close_fail:
	libusb_close(usb->dev);
fail:
	free(usb);
	return NULL;
}

void usb_close(usb_handle *usb) {
	libusb_close(usb->dev);
	free(usb);
}

int usb_ctrl(usb_handle *usb, void *data,
	uint8_t typ, uint8_t req, uint16_t val, uint16_t idx, uint16_t len) {
	int r = libusb_control_transfer(usb->dev, typ, req, val, idx, data, len, 5000);
	if (r < 0) {
		return -1;
	} else {
		return r;
	}
}

int usb_read(usb_handle *usb, void *data, int len) {
	int xfer = len;
	int r = libusb_bulk_transfer(usb->dev, usb->ei, data, len, &xfer, 5000);
	if (r < 0) {
		return -1;
	}
	return xfer;
}

int usb_write(usb_handle *usb, const void *data, int len) {
	int xfer = len;
	int r = libusb_bulk_transfer(usb->dev, usb->eo, (void*) data, len, &xfer, 5000);
	if (r < 0) {
		return -1;
	}
	return xfer;
}

