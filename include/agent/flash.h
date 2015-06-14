// agent/flash.h
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

#ifndef _AGENT_FLASH_H_
#define _AGENT_FLASH_H_

typedef unsigned int u32;

#define AGENT_MAGIC 0x42776166
#define AGENT_VERSION 0x00010000

typedef struct flash_agent {
	u32 magic;
	u32 version;
	u32 flags;
        u32 load_addr;

        u32 data_addr;
	u32 data_size; // bytes
	u32 flash_addr;
	u32 flash_size; // bytes

	u32 reserved0;
	u32 reserved1;
	u32 reserved2;
	u32 reserved3;

#ifdef _AGENT_HOST_
	u32 setup;
	u32 erase;
	u32 write;
	u32 ioctl;
#else
	int (*setup)(struct flash_agent *agent);
	int (*erase)(u32 flash_addr, u32 length);
	int (*write)(u32 flash_addr, const void *data, u32 length);
	int (*ioctl)(u32 op, void *ptr, u32 arg0, u32 arg1);
#endif
} flash_agent;

#ifndef _AGENT_HOST_
int flash_agent_setup(flash_agent *agent);
int flash_agent_erase(u32 flash_addr, u32 length);
int flash_agent_write(u32 flash_addr, const void *data, u32 length);
int flash_agent_ioctl(u32 op, void *ptr, u32 arg0, u32 arg1);
#endif

#define ERR_NONE	0
#define ERR_FAIL	-1
#define ERR_INVALID	-2
#define ERR_ALIGNMENT	-3

#define FLAG_WSZ_256B		0x00000100
#define FLAG_WSZ_512B		0x00000200
#define FLAG_WSZ_1K		0x00000400
#define FLAG_WSZ_2K		0x00000800
#define FLAG_WSZ_4K		0x00001000
// optional hints as to underlying write block sizes

#define FLAG_BOOT_ROM_HACK	0x00000001
// Allow a boot ROM to run after RESET by setting a watchpoint
// at 0 and running until the watchpoint is hit.  Necessary on
// some parts which require boot ROM initialization of Flash
// timing registers, etc.


// Flash agent binaries will be downloaded to device memory at
// fa.load_addr.  The memory below this address will be used as
// the stack for method calls.  It should be sized appropriately.
//
// The fa.magic field will be replaced with 0xbe00be00 (two
// Thumb BKPT instructions) before download to device.
//
// Calling convention for flash_agent methods:
// 1. SP will be set to fa.load_addr - 4
// 2. LR will be set to fa.load_addr
// 3. PC will be set to the method address
// 4. R0..R3 will be loaded with method arguments
// 5. Processor will be resumed (in Privileged Thread mode)
// 6. Upon processor entry to debug mode
//    a. if PC == fa.base_addr, status read from R0
//    b. else status = fatal error


// setup() must be invoked after processor reset and before any
// other methods are invoked.  The fields data_addr, data_size,
// flash_addr, and flash_size are read back after this method
// returns success (to allow it to dynamically size flash based
// on part ID, etc).

// fa.data_size must be a multiple of the minimum block size that
// fa.write requires, so that if the host has to issue a series
// of fa.write calls to do a larger-than-data-buffer-sized flash write,
// subsequent write calls will have appropriate alignment.
//
// fa.write() may round writes up the the minimum write block size
// as necessary, and should fill with 0s in that case.*
// 
// fa.write() behaviour is undefined if the underlying flash has not
// been erased first.  It may fail (ERR_FAIL) or it may appear to 
// succeed, but flash contents may be incorrect.
//
// fa.write() may fail (ERR_ALIGN) if the start address is not aligned
// with a flash page or block.*
//
// fa.erase() may round up to the next erasure block size if necessary
// to ensure the requested region is erased.*
//
// fa.erase(fa.flash_addr, fa.flash_size) should cause a full erase if
// possible.
//
// fa.ioctl() must return ERR_INVALID if op is unsupported.
// Currently no ops are defined, but OTP/EEPROM/Config bits are
// planned to be managed with ioctls.
//
// Bogus parameters may cause failure (ERR_INVALID)
//
// * In general, conveying the full complexity of embedded flash 
// configuration (which could include various banks of various sizes,
// with differing write and erase block size requirements) is not
// attempted.  The goal is to provide an agent that can reasonably
// handle reasonable flash requests (eg the user knows what a sane
// starting alignment, etc is, does not split logical "partitions" 
// across physical erase block boundaries, etc)

#endif
