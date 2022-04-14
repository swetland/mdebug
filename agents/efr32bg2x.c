// agents/efr32bg2x.c
//
// Copyright 2022 Brian Swetland <swetland@frotz.net>
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

#define CMU_CLKEN1_SET 0x40009068
#define CMU_CLKEN1_MSC     (1U<<17)

#define MSC_WRITECTRL      0x4003000C
#define MSC_WRITECTRL_SET  0x4003100C
#define MSC_WRITECTRL_CLR  0x4003200C
#define MSC_WRITECMD       0x40030010
#define MSC_WRITECMD_SET   0x40031010
#define MSC_WRITECMD_CLR   0x40032010
#define MSC_ADDRB          0x40030014
#define MSC_WDATA          0x40030018
#define MSC_STATUS         0x4003001C

#define MSC_WRITECTRL_WREN 1

#define MSC_WRITECMD_WRITEND   (1U<<2)
#define MSC_WRITECMD_ERASEPAGE (1U<<1)

#define MSC_STATUS_WREADY         (1U<<27)
#define MSC_STATUS_PWRON          (1U<<24)
#define MSC_STATUS_REGLOCK        (1U<<16)
#define MSC_STATUS_TIMEOUT        (1U<<6)
#define MSC_STATUS_PENDING        (1U<<5)
#define MSC_STATUS_ERASEABORTYED  (1U<<4)
#define MSC_STATUS_WDATAREADY     (1U<<3)
#define MSC_STATUS_INVADDR        (1U<<2)
#define MSC_STATUS_LOCKED         (1U<<1)
#define MSC_STATUS_BUSY           (1U<<0)

static unsigned FLASH_PAGE_SIZE = 8192;
static unsigned FLASH_SIZE = 352 * 1024;

int flash_agent_setup(flash_agent *agent) {
	// TODO - validate part ID
	if (0) {
		// unknown part
		return ERR_INVALID;
	}

	// TODO: read from userdata
	agent->flash_size = FLASH_SIZE;

	return ERR_NONE;
}

int flash_agent_erase(u32 flash_addr, u32 length) {
	unsigned v;
	int status = ERR_NONE;
	if (flash_addr > FLASH_SIZE) {
		return ERR_INVALID;
	}
	if (flash_addr & (FLASH_PAGE_SIZE - 1)) {
		return ERR_ALIGNMENT;
	}

	writel(CMU_CLKEN1_MSC, CMU_CLKEN1_SET);
	writel(MSC_WRITECTRL_WREN, MSC_WRITECTRL_SET);
	while (length > 0) {
		writel(flash_addr, MSC_ADDRB);
		v = readl(MSC_STATUS);
		if (v & (MSC_STATUS_INVADDR | MSC_STATUS_LOCKED)) {
			status = ERR_INVALID;
			break;
		}
		if (!(v & MSC_STATUS_WREADY)) {
			status = ERR_FAIL;
			break;
		}
		if (length > FLASH_PAGE_SIZE) {
			length -= FLASH_PAGE_SIZE;
		} else {
			length = 0;
		}
		writel(MSC_WRITECMD_ERASEPAGE, MSC_WRITECMD_SET);
		while (readl(MSC_STATUS) & MSC_STATUS_BUSY) ;
		for (unsigned n = 0; n < FLASH_PAGE_SIZE; n += 4) {
			if (readl(flash_addr + n) != 0xFFFFFFFF) {
				status = ERR_FAIL;
				goto done;
			}
		}
		flash_addr += FLASH_PAGE_SIZE;
	}
done:
	writel(MSC_WRITECTRL_WREN, MSC_WRITECTRL_CLR);
	return status;
}

int flash_agent_write(u32 flash_addr, const void *_data, u32 length) {
	int status = ERR_NONE;
	const unsigned *data = _data;
	unsigned v;
	if (flash_addr > FLASH_SIZE) {
		return ERR_INVALID;
	}
	if ((flash_addr & 3) || (length & 3)) {
		return ERR_ALIGNMENT;
	}
	writel(CMU_CLKEN1_MSC, CMU_CLKEN1_SET);
	writel(MSC_WRITECTRL_WREN, MSC_WRITECTRL_SET);
	while (length > 0) {
		writel(flash_addr, MSC_ADDRB);
		v = readl(MSC_STATUS);
		if (v & (MSC_STATUS_INVADDR | MSC_STATUS_LOCKED)) {
			status = ERR_INVALID;
			break;
		}
		if (!(v & MSC_STATUS_WREADY)) {
			status = ERR_FAIL;
			break;
		}
		writel(*data, MSC_WDATA);
		while (readl(MSC_STATUS) & MSC_STATUS_BUSY) ;
		if (readl(flash_addr) != *data) {
			status = ERR_FAIL;
			goto done;
		}
		data++;
		length -= 4;
		flash_addr += 4;
	}
done:
	writel(MSC_WRITECTRL_WREN, MSC_WRITECTRL_CLR);
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
