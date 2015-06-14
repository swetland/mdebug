// agent-lpc15xx/main.c
//
// Copyright 2015 Brian Swetland <swetland@frotz.net>
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <agent/flash.h>

#define LPC_IAP_FUNC	0x03000201
#define LPC_IAP_PREPARE	50
#define LPC_IAP_WRITE	51
#define LPC_IAP_ERASE	52

void (*romcall)(u32 *param, u32 *status) = (void*) LPC_IAP_FUNC;

int flash_agent_setup(flash_agent *agent) {
	return ERR_NONE;
}

int flash_agent_erase(u32 flash_addr, u32 length) {
	u32 p[5];
	u32 page = flash_addr >> 12;
	u32 last = page + ((length - 1) >> 12);
	if (flash_addr & 0xFFF) {
		return ERR_ALIGNMENT;
	}

	p[0] = LPC_IAP_PREPARE;
	p[1] = page;
	p[2] = last;
	romcall(p,p);
	if (p[0]) {
		return ERR_FAIL;
	}

	p[0] = LPC_IAP_ERASE;
	p[1] = page;
	p[2] = last;
	p[3] = 0x2ee0;
	romcall(p,p);
	if (p[0]) {
		return ERR_FAIL;
	}
	return ERR_NONE;
}

int flash_agent_write(u32 flash_addr, const void *data, u32 length) {
	u32 p[5];
	u32 page = flash_addr >> 12;
	if (flash_addr & 0xFFF) {
		return ERR_ALIGNMENT;
	}

	p[0] = LPC_IAP_PREPARE;
	p[1] = page;
	p[2] = page;
	romcall(p,p);
	if (p[0]) {
		return ERR_FAIL;
	}

	p[0] = LPC_IAP_WRITE;
	p[1] = flash_addr;
	p[2] = (u32) data;
	p[3] = 0x1000;
	p[4] = 0x2ee0;
	romcall(p,p);
	if (p[0]) {
		return ERR_FAIL;
	}

	return ERR_NONE;
}

int flash_agent_ioctl(u32 op, void *ptr, u32 arg0, u32 arg1) {
	return ERR_INVALID;
}


