// agents/pico.c
//
// Copyright 2020 Brian Swetland <swetland@frotz.net>
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

#include <stdint.h>
#include <agent/flash.h>
#include <fw/io.h>

static unsigned FLASH_BLOCK_SIZE = 256;
static unsigned FLASH_PAGE_SIZE = 4096;
static unsigned FLASH_SIZE = 4 * 1024 * 1024;

#define FLASH_XIP_BASE 0x10000000

#define CODE(c1, c2) (((c2) << 8) | (c1))

#define ROM_LOOKUP_FN_PTR 0x18
#define ROM_FN_TABLE_PTR 0x14

// lookup fn for code using ROM lookup helper and ROM fn table
void* lookup_fn(uint32_t code) {
	void* (*lookup)(uint32_t table, uint32_t code) =
		(void*) (uintptr_t) *((uint16_t*) ROM_LOOKUP_FN_PTR);

	return lookup(*((uint16_t*)ROM_FN_TABLE_PTR), code);
}

static void (*_flash_connect)(void);
static void (*_flash_exit_xip)(void);
static void (*_flash_erase)(uint32_t addr, uint32_t len,
		uint32_t block_size, uint32_t block_cmd);
static void (*_flash_write)(uint32_t addr, const void* data, uint32_t len);
static void (*_flash_flush_cache)(void);
static void (*_flash_enter_xip)(void);

// erase: addr and count must be 4096 aligned
// write: addr and len must be 256 aligned 

int flash_agent_setup(flash_agent *agent) {
	// TODO - validate part ID
	if (0) {
		// unknown part
		return ERR_INVALID;
	}

	_flash_connect = lookup_fn(CODE('I','F'));
	_flash_exit_xip = lookup_fn(CODE('E','X'));
	_flash_erase = lookup_fn(CODE('R','E'));
	_flash_write = lookup_fn(CODE('R','P'));
	_flash_flush_cache = lookup_fn(CODE('F','C'));
	_flash_enter_xip = lookup_fn(CODE('C','X'));

	// TODO: obtain from spi flash	
	agent->flash_size = FLASH_SIZE;

	return ERR_NONE;
}

int flash_agent_erase(u32 flash_addr, u32 length) {
	if (flash_addr > FLASH_SIZE) {
		return ERR_INVALID;
	}
	if (flash_addr & (FLASH_PAGE_SIZE - 1)) {
		return ERR_ALIGNMENT;
	}
	if (length & (FLASH_PAGE_SIZE-1)) {
		length = (length & (~(FLASH_PAGE_SIZE-1))) + FLASH_PAGE_SIZE;
	}

	_flash_connect();
	_flash_exit_xip();
	_flash_erase(flash_addr, length, 65536, 0xD8);
	_flash_flush_cache();
	_flash_enter_xip();

	uint32_t* flash = (void*) (flash_addr + FLASH_XIP_BASE);
	for (uint32_t n = 0; n < length; n+= 4) {
		if (*flash++ != 0xFFFFFFFF) {
			return ERR_FAIL;
		}
	}

	return ERR_NONE;
}

int flash_agent_write(u32 flash_addr, const void *data, u32 length) {
	if (flash_addr > FLASH_SIZE) {
		return ERR_INVALID;
	}
	if (flash_addr & (FLASH_BLOCK_SIZE - 1)) {
		return ERR_ALIGNMENT;
	}
	if (length & (FLASH_BLOCK_SIZE-1)) {
		length = (length & (~(FLASH_BLOCK_SIZE-1))) + FLASH_BLOCK_SIZE;
	}

	_flash_connect();
	_flash_exit_xip();
	_flash_write(flash_addr, data, length);
	_flash_flush_cache();
	_flash_enter_xip();

	uint32_t* flash = (void*) (flash_addr + FLASH_XIP_BASE);
	const uint32_t* xdata = data;
	for (uint32_t n = 0; n < length; n += 4) {
		if (*flash++ != *xdata++) {
			return ERR_FAIL;
		}
	}

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
	.data_size =	0x4000,
	.flash_addr =	FLASH_BASE,
	.flash_size =	0,
	.setup =	flash_agent_setup,
	.erase =	flash_agent_erase,
	.write =	flash_agent_write,
	.ioctl =	flash_agent_ioctl,
};
