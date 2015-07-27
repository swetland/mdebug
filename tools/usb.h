/* usb.h
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

#ifndef _USB_H_
#define _USB_H_

#include <stdint.h>

typedef struct usb_handle usb_handle;

/* simple usb api for devices with bulk in+out interfaces */

usb_handle *usb_open(unsigned vid, unsigned pid, unsigned ifc);
void usb_close(usb_handle *usb);
int usb_read(usb_handle *usb, void *data, int len);
int usb_write(usb_handle *usb, const void *data, int len);
int usb_ctrl(usb_handle *usb, void *data,
	uint8_t typ, uint8_t req, uint16_t val, uint16_t idx, uint16_t len);
#endif
