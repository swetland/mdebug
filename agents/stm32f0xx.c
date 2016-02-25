// agents/stm-main.c
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

#define LOADADDR	0x20000400

#define FLASH_BASE	0x08000000
#define FLASH_SIZE	0x00004000

#include <agent/flash.h>
#include <fw/io.h>

#define _FLASH_BASE		0x40022000
#define FLASH_ACR		(_FLASH_BASE + 0x00)

#define FLASH_KEYR		(_FLASH_BASE + 0x04)
#define FLASH_KEYR_KEY1		0x45670123
#define FLASH_KEYR_KEY2		0xCDEF89AB

#define FLASH_SR		(_FLASH_BASE + 0x0C)
#define FLASH_SR_EOP		(1 << 5) // end of operation
#define FLASH_SR_WP_ERR		(1 << 4) // write protect error
#define FLASH_SR_PG_ERR		(1 << 2) // programming error
#define FLASH_SR_BSY		(1 << 0) // busy
#define FLASH_SR_ERR_MASK	(FLASH_SR_WP_ERR | FLASH_SR_PG_ERR)

#define FLASH_CR		(_FLASH_BASE + 0x10)
#define FLASH_CR_OBL_LAUNCH	(1 << 13) // reload option byte & reset system
#define FLASH_CR_EOPIE		(1 << 12) // enable end-of-op irq
#define FLASH_CR_ERRIE		(1 << 10) // enable error irq
#define FLASH_CR_OPTWRE		(1 << 9) // option byte write enable
#define FLASH_CR_LOCK		(1 << 7) // indicates flash is locked
#define FLASH_CR_STRT		(1 << 6) // start erase operation
#define FLASH_CR_OPTER		(1 << 5) // option byte erase
#define FLASH_CR_OPTPG		(1 << 4) // option byte program
#define FLASH_CR_MER		(1 << 2) // mass erase
#define FLASH_CR_PER		(1 << 1) // page erase
#define FLASH_CR_PG		(1 << 0) // programming
#define FLASH_CR_MASK		(~0x000036F7) // bits to not modify

#define FLASH_AR		(_FLASH_BASE + 0x14)

static unsigned FLASH_PAGE_SIZE = 1024;

int flash_agent_setup(flash_agent *agent) {

	// check MCU ID
	switch (readl(0x40015800) & 0xFFF) {
	case 0x444: // F03x
	case 0x445: // F04x
	case 0x440: // F05x
		break;
	case 0x448: // F07x
	case 0x442: // F09x
		FLASH_PAGE_SIZE = 2048;
		break;
	default:
		// unknown part
		return ERR_INVALID;
	}

	// check flash size
	agent->flash_size = readw(0x1FFFF7CC) * 1024;

	writel(FLASH_KEYR_KEY1, FLASH_KEYR);
	writel(FLASH_KEYR_KEY2, FLASH_KEYR);
	if (readl(FLASH_CR) & FLASH_CR_LOCK) {
		return ERR_FAIL;
	} else {
		return ERR_NONE;
	}
}

int flash_agent_erase(u32 flash_addr, u32 length) {
	u32 cr;
	int n;
	if (flash_addr & (FLASH_PAGE_SIZE - 1)) {
		return ERR_ALIGNMENT;
	}
	cr = readl(FLASH_CR) & FLASH_CR_MASK;

	while (length > 0) {
		if (length > FLASH_PAGE_SIZE) {
			length -= FLASH_PAGE_SIZE;
		} else {
			length = 0;
		}
		writel(cr | FLASH_CR_PER, FLASH_CR);
		writel(flash_addr, FLASH_AR);
		writel(cr | FLASH_CR_PER | FLASH_CR_STRT, FLASH_CR);
		while (readl(FLASH_SR) & FLASH_SR_BSY) ;
		for (n = 0; n < FLASH_PAGE_SIZE; n += 4) {
			if (readl(flash_addr + n) != 0xFFFFFFFF) {
				return ERR_FAIL;
			}
		}
		flash_addr += FLASH_PAGE_SIZE;
	}
	return ERR_NONE;
}

int flash_agent_write(u32 flash_addr, const void *_data, u32 length) {
	const unsigned short *data = _data;
	u32 v, cr;
	if ((flash_addr & 3) || (length & 3)) {
		return ERR_ALIGNMENT;
	}
	cr = readl(FLASH_CR) & FLASH_CR_MASK;
	writel(cr | FLASH_CR_PG, FLASH_CR);
	while (length > 0) {
		writew(*data, flash_addr);
		while ((v = readl(FLASH_SR)) & FLASH_SR_BSY) ;
		if (readw(flash_addr) != *data) {
			writel(cr, FLASH_CR);
			return ERR_FAIL;
		}
		data++;
		length -= 2;
		flash_addr += 2;
	}
	writel(cr, FLASH_CR);
	return ERR_NONE;
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
	.data_size =	0x1000,
	.flash_addr =	FLASH_BASE,
	.flash_size =	FLASH_SIZE,
	.setup =	flash_agent_setup,
	.erase =	flash_agent_erase,
	.write =	flash_agent_write,
	.ioctl =	flash_agent_ioctl,
};
