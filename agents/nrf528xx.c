// agents/nrf528xx.c
//
// Copyright 2019 Brian Swetland <swetland@frotz.net>
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

#define LOADADDR	0x20000400

#define FLASH_BASE	0x00000000

#include <agent/flash.h>
#include <fw/io.h>

#define NVMC_READY		0x4001E400
#define NVMC_CONFIG		0x4001E504
#define NVMC_CONFIG_READ	0
#define NVMC_CONFIG_WRITE	1
#define NVMC_CONFIG_ERASE	2
#define NVMC_ERASEPAGE		0x4001E508

#define FICR_CODEPAGESIZE	0x10000010
#define FICR_CODESIZE           0x10000014

static unsigned FLASH_PAGE_SIZE = 1024;
static unsigned FLASH_SIZE = 192 * 1024;

int flash_agent_setup(flash_agent *agent) {

	// TODO - validate part ID
	if (0) {
		// unknown part
		return ERR_INVALID;
	}

	// check flash size
	FLASH_PAGE_SIZE = readl(FICR_CODEPAGESIZE);
	FLASH_SIZE = FLASH_PAGE_SIZE * readl(FICR_CODESIZE);
	
	agent->flash_size = FLASH_SIZE;

	return ERR_NONE;
}

int flash_agent_erase(u32 flash_addr, u32 length) {
	int status = ERR_NONE;
	if (flash_addr > FLASH_SIZE) {
		return ERR_INVALID;
	}
	if (flash_addr & (FLASH_PAGE_SIZE - 1)) {
		return ERR_ALIGNMENT;
	}
	writel(NVMC_CONFIG_ERASE, NVMC_CONFIG);
	while (length > 0) {
		if (length > FLASH_PAGE_SIZE) {
			length -= FLASH_PAGE_SIZE;
		} else {
			length = 0;
		}
		writel(flash_addr, NVMC_ERASEPAGE);
		while (readl(NVMC_READY) != 1) ;
		for (unsigned n = 0; n < FLASH_PAGE_SIZE; n += 4) {
			if (readl(flash_addr + n) != 0xFFFFFFFF) {
				status = ERR_FAIL;
				goto done;
			}
		}
		flash_addr += FLASH_PAGE_SIZE;
	}
done:
	writel(NVMC_CONFIG_READ, NVMC_CONFIG);
	return status;
}

int flash_agent_write(u32 flash_addr, const void *_data, u32 length) {
	int status = ERR_NONE;
	const unsigned *data = _data;
	if (flash_addr > FLASH_SIZE) {
		return ERR_INVALID;
	}
	if ((flash_addr & 3) || (length & 3)) {
		return ERR_ALIGNMENT;
	}
	writel(NVMC_CONFIG_WRITE, NVMC_CONFIG);
	while (length > 0) {
		writel(*data, flash_addr);
		while (readl(NVMC_READY) != 1) ;
		if (readl(flash_addr) != *data) {
			status = ERR_FAIL;
			goto done;
		}
		data++;
		length -= 4;
		flash_addr += 4;
	}
done:
	writel(NVMC_CONFIG_READ, NVMC_CONFIG);
	return status;
}

int flash_agent_ioctl(u32 op, void *ptr, u32 arg0, u32 arg1) {
	return ERR_INVALID;
}

const flash_agent __attribute((section(".vectors"))) FlashAgent = {
	.magic =	AGENT_MAGIC,
	.version =	AGENT_VERSION,
	.flags =	0,
	.load_addr =	LOADADDR,
	.data_addr =	LOADADDR + 0x400,
	.data_size =	0x4000,
	.flash_addr =	FLASH_BASE,
	.flash_size =	0,
	.setup =	flash_agent_setup,
	.erase =	flash_agent_erase,
	.write =	flash_agent_write,
	.ioctl =	flash_agent_ioctl,
};
