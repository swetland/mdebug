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

#ifdef CONFIG_ARCH_LPC15XX
#define LPC_IAP_FUNC	0x03000205
#else
#define LPC_IAP_FUNC	0x1fff1ff1
#endif

#define LPC_IAP_PREPARE	50
#define LPC_IAP_WRITE	51
#define LPC_IAP_ERASE	52

// Note that while the databook claims you can reuse the same array
// for both parameters and results, this is a lie.  Attempting to 
// do so causes an invalid command failure.


int flash_agent_setup(flash_agent *agent) {
	return ERR_NONE;
}

int flash_agent_erase(u32 flash_addr, u32 length) {
	void (*romcall)(u32 *, u32 *) = (void*) LPC_IAP_FUNC;
	u32 p[5],r[4];
	u32 page = flash_addr >> 12;
	u32 last = page + ((length - 1) >> 12);
	if (flash_addr & 0xFFF) {
		return ERR_ALIGNMENT;
	}

	p[0] = LPC_IAP_PREPARE;
	p[1] = page;
	p[2] = last;
	romcall(p,r);
	if (r[0]) {
		return ERR_FAIL;
	}

	p[0] = LPC_IAP_ERASE;
	p[1] = page;
	p[2] = last;
	p[3] = 0x2ee0;
	romcall(p,r);
	if (r[0]) {
		return ERR_FAIL;
	}
	return ERR_NONE;
}

int flash_agent_write(u32 flash_addr, const void *data, u32 length) {
	void (*romcall)(u32 *,u32 *) = (void*) LPC_IAP_FUNC;
	u32 p[5],r[4];
	u32 page = flash_addr >> 12;
	if (flash_addr & 0xFFF) {
		return ERR_ALIGNMENT;
	}

	p[0] = LPC_IAP_PREPARE;
	p[1] = page;
	p[2] = page;
	romcall(p,r);
	if (r[0]) {
		return ERR_FAIL;
	}

	// todo: smaller writes, etc
	if (length != 4096) {
		int n;
		for (n = length; n < 4096; n++) {
			((char*) data)[n] = 0;
		}
	}
	p[0] = LPC_IAP_WRITE;
	p[1] = flash_addr;
	p[2] = (u32) data;
	p[3] = 0x1000;
	p[4] = 0x2ee0;
	romcall(p,r);
	if (r[0]) {
		return ERR_FAIL;
	}

	return ERR_NONE;
}

int flash_agent_ioctl(u32 op, void *ptr, u32 arg0, u32 arg1) {
	return ERR_INVALID;
}

const flash_agent __attribute((section(".vectors"))) FlashAgent = {
	.magic =	AGENT_MAGIC,
	.version =	AGENT_VERSION,
	.flags =	FLAG_BOOT_ROM_HACK,
	.load_addr =	CONFIG_LOADADDR,
	.data_addr =	CONFIG_LOADADDR + 0x400,
	.data_size =	0x1000,
	.flash_addr =	CONFIG_FLASHADDR,
	.flash_size =	CONFIG_FLASHSIZE,
	.setup =	flash_agent_setup,
	.erase =	flash_agent_erase,
	.write =	flash_agent_write,
	.ioctl =	flash_agent_ioctl,
};
