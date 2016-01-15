// agent-lpc15xx/main.c
//
// Copyright 2016 Brian Swetland <swetland@frotz.net>
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
#include "cc13xx-romapi.h"

int flash_agent_setup(flash_agent *agent) {
	return ERR_NONE;
}

int flash_agent_erase(u32 flash_addr, u32 length) {
	if (flash_addr & 0xFFF) {
		return ERR_ALIGNMENT;
	}
	while (length > 0) {
		if (ROM_FlashSectorErase(flash_addr)) {
			return ERR_FAIL;
		}
		if (length > 4096) {
			length -= 4096;
			flash_addr += 4096;
		} else {
			break;
		}
	}
	return 0;
}

int flash_agent_write(u32 flash_addr, const void *data, u32 length) {
	if (flash_addr & 0xFFF) {
		return ERR_ALIGNMENT;
	}
	if (ROM_FlashProgram(data, flash_addr, length)) {
		return ERR_FAIL;
	}
	return 0;
}


int flash_agent_ioctl(u32 op, void *ptr, u32 arg0, u32 arg1) {
	return ERR_INVALID;
}

const flash_agent __attribute((section(".vectors"))) FlashAgent = {
	.magic =	AGENT_MAGIC,
	.version =	AGENT_VERSION,
	.flags =	0,
	.load_addr =	0x20000400,
	.data_addr =	0x20001000,
	.data_size =	0x1000,
	.flash_addr =	0x00000000,
	.flash_size =	0x00020000,
	.setup =	flash_agent_setup,
	.erase =	flash_agent_erase,
	.write =	flash_agent_write,
	.ioctl =	flash_agent_ioctl,
};
