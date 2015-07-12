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

#include <agent/flash.h>
#include <fw/io.h>

#define FLASH_BASE		0x40023C00
#define FLASH_ACR		(FLASH_BASE + 0x00)

#define FLASH_KEYR		(FLASH_BASE + 0x04)
#define FLASH_KEYR_KEY1		0x45670123
#define FLASH_KEYR_KEY2		0xCDEF89AB

#define FLASH_SR		(FLASH_BASE + 0x0C)
#define FLASH_SR_BSY		(1 << 16)
#define FLASH_SR_PGSERR		(1 << 7) // sequence error
#define FLASH_SR_PGPERR		(1 << 6) // parallelism error
#define FLASH_SR_PGAERR		(1 << 5) // alignment error
#define FLASH_SR_WRPERR		(1 << 4) // write-protect error
#define FLASH_SR_OPERR		(1 << 1) // operation error
#define FLASH_SR_ERRMASK	0xF2
#define FLASH_SR_EOP		(1 << 0) // end of operation

#define FLASH_CR		(FLASH_BASE + 0x10)
#define FLASH_CR_LOCK		(1 << 31)
#define FLASH_CR_ERRIE		(1 << 25) // error irq en
#define FLASH_CR_EOPIE		(1 << 24) // end of op irq en
#define FLASH_CR_STRT		(1 << 16) // start
#define FLASH_CR_PSIZE_8	(0 << 8)
#define FLASH_CR_PSIZE_16	(1 << 8)
#define FLASH_CR_PSIZE_32	(2 << 8)
#define FLASH_CR_PSIZE_64	(3 << 8)
#define FLASH_CR_SNB(n)		(((n) & 15) << 3) // sector number
#define FLASH_CR_MER		(1 << 2) // mass erase
#define FLASH_CR_SER		(1 << 1) // sector erase
#define FLASH_CR_PG		(1 << 0) // programming


#define SECTORS 12

static u32 sectors[SECTORS + 1] = {
	0x00000000,
	0x00004000,
	0x00008000,
	0x0000C000,
	0x00010000,
	0x00020000,
	0x00040000,
	0x00060000,
	0x00080000,
	0x000A0000,
	0x000C0000,
	0x000E0000,
	0x00100000,
};

int flash_agent_setup(flash_agent *agent) {
	writel(FLASH_KEYR_KEY1, FLASH_KEYR);
	writel(FLASH_KEYR_KEY2, FLASH_KEYR);
	if (readl(FLASH_CR) & FLASH_CR_LOCK) {
		return ERR_FAIL;
	} else {
		return ERR_NONE;
	}
}

int flash_agent_erase(u32 flash_addr, u32 length) {
	u32 v;
	int n;
	for (n = 0; n < SECTORS; n++) {
		if (flash_addr == sectors[n]) goto ok;
	}
	return ERR_ALIGNMENT;
ok:
	for (;;) {
		writel(FLASH_CR_SER | FLASH_CR_SNB(n), FLASH_CR);
		writel(FLASH_CR_STRT | FLASH_CR_SER | FLASH_CR_SNB(n), FLASH_CR);
		while ((v = readl(FLASH_SR)) & FLASH_SR_BSY) ;
		if (v & FLASH_SR_ERRMASK) {
			return ERR_FAIL;
		}
		n++;
		if (n == SECTORS) break;
		if ((sectors[n] - flash_addr) >= length) break;
	}
	return ERR_NONE;
}

int flash_agent_write(u32 flash_addr, const void *_data, u32 length) {
	const u32 *data = _data;
	u32 v;
	if ((flash_addr & 3) || (length & 3)) {
		return ERR_ALIGNMENT;
	}
	writel(FLASH_CR_PG | FLASH_CR_PSIZE_32, FLASH_CR);
	while (length > 0) {
		writel(*data, flash_addr);
		data++;
		length -= 4;
		flash_addr += 4;
		while ((v = readl(FLASH_SR)) & FLASH_SR_BSY) ;
		if (v & FLASH_SR_ERRMASK) {
			writel(0, FLASH_CR);
			return ERR_FAIL;
		}
	}
	writel(0, FLASH_CR);
	return ERR_NONE;
}

int flash_agent_ioctl(u32 op, void *ptr, u32 arg0, u32 arg1) {
	return ERR_INVALID;
}

const flash_agent __attribute((section(".vectors"))) FlashAgent = {
	.magic =	AGENT_MAGIC,
	.version =	AGENT_VERSION,
	.flags =	0,
	.load_addr =	CONFIG_LOADADDR,
	.data_addr =	CONFIG_LOADADDR + 0x400,
	.data_size =	0x8000,
	.flash_addr =	CONFIG_FLASHADDR,
	.flash_size =	CONFIG_FLASHSIZE,
	.setup =	flash_agent_setup,
	.erase =	flash_agent_erase,
	.write =	flash_agent_write,
	.ioctl =	flash_agent_ioctl,
};
