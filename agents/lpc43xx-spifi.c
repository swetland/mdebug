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
#include <fw/io.h>

// ---- pinmux

#define PIN_CFG(m,n)	(0x40086000 + ((m) * 0x80) + ((n) * 4))
#define PIN_MODE(n)	((n) & 3)
#define PIN_PULLUP	(0 << 3) // pull-up, no pull-down
#define PIN_REPEATER	(1 << 3) // repeater mode
#define PIN_PLAIN	(2 << 3) // no pull-up, no pull-down
#define PIN_PULLDOWN	(3 << 3) // pull-down, no pull-up
#define PIN_SLOW	(0 << 5) // slow slew rate (low noise, medium speed)
#define PIN_FAST	(1 << 5) // fast slew rate (medium noise, fast speed)
#define PIN_INPUT	(1 << 6) // enable input buffer, required for inputs
#define PIN_FILTER	(1 << 7) // enable glitch filter, not for >30MHz signals

// ---- spifi serial flash controller

#define SPIFI_CTRL		0x40003000 // Control
#define SPIFI_CMD		0x40003004 // Command
#define SPIFI_ADDR		0x40003008 // Address
#define SPIFI_IDATA		0x4000300C // Intermediate Data
#define SPIFI_CLIMIT		0x40003010 // Cache Limit
#define SPIFI_DATA		0x40003014 // Data
#define SPIFI_MCMD		0x40003018 // Memory Command
#define SPIFI_STAT		0x4000301C // Status

#define CTRL_TIMEOUT(n)		((n) & 0xFFFF)
#define CTRL_CSHIGH(n)		(((n) & 0xF) << 16) // Minimum /CS high time (serclks - 1)
#define CTRL_D_PRFTCH_DIS	(1 << 21) // Disable Prefetch of Data
#define CTRL_INTEN		(1 << 22) // Enable IRQ on end of command
#define CTRL_MODE3		(1 << 23) // 0=SCK low after +edge of last bit, 1=high
#define CTRL_PRFTCH_DIS		(1 << 27) // Disable Prefetch
#define CTRL_DUAL		(1 << 28) // 0=Quad 1=Dual (bits in "wide" ops)
#define CTRL_QUAD		(0 << 28)
#define CTRL_RFCLK		(1 << 29) // 1=sample read data on -edge clock
#define CTRL_FBCLK		(1 << 30) // use feedback clock from SCK pin for sampling
#define CTRL_DMAEN		(1 << 31) // enable DMA request output

#define CMD_DATALEN(n)		((n) & 0x3FFF)
#define CMD_POLL		(1 << 14) // if set, read byte repeatedly until condition
#define CMD_POLLBIT(n)		((n) & 7) // which bit# to check
#define CMD_POLLSET		(1 << 3)  // condition is bit# set
#define CMD_POLLCLR		(0 << 3)  // condition is bit# clear
#define CMD_DOUT		(1 << 15) // 1=data phase output, 0=data phase input
#define CMD_DIN			(0 << 15)
#define CMD_INTLEN(n)		(((n) & 7) << 16) // count of intermediate bytes
#define CMD_FF_SERIAL		(0 << 19) // all command fields serial
#define CMD_FF_WIDE_DATA	(1 << 19) // data is wide, all other fields serial
#define CMD_FF_SERIAL_OPCODE	(2 << 19) // opcode is serial, all other fields wide
#define CMD_FF_WIDE		(3 << 19) // all command fields wide
#define CMD_FR_OP		(1 << 21) // frame format: opcode only
#define CMD_FR_OP_1B		(2 << 21) // opcode, lsb addr
#define CMD_FR_OP_2B		(3 << 21) // opcode, 2 lsb addr
#define CMD_FR_OP_3B		(4 << 21) // opcode, 3 lsb addr
#define CMD_FR_OP_4B		(5 << 21) // opcode, 4b address
#define CMD_FR_3B		(6 << 21) // 3 lsb addr
#define CMD_FR_4B		(7 << 21) // 4 lsb addr
#define CMD_OPCODE(n)		((n) << 24)

#define STAT_MCINIT		(1 << 0) // set on sw write to MCMD, clear on RST, wr(0)
#define STAT_CMD		(1 << 1) // set when CMD written, clear on CS, RST
#define STAT_RESET		(1 << 4) // write 1 to abort current txn or memory mode
#define STAT_INTRQ		(1 << 5) // read IRQ status, wr(1) to clear

#define CMD_PAGE_PROGRAM		0x02
#define CMD_READ_DATA			0x03
#define CMD_READ_STATUS			0x05
#define CMD_WRITE_ENABLE		0x06
#define CMD_SECTOR_ERASE		0x20

static void spifi_write_enable(void) {
	writel(CMD_FF_SERIAL | CMD_FR_OP | CMD_OPCODE(CMD_WRITE_ENABLE),
		SPIFI_CMD);
	while (readl(SPIFI_STAT) & STAT_CMD) ;
}

static void spifi_wait_busy(void) {
	while (readl(SPIFI_STAT) & STAT_CMD) ;
	writel(CMD_POLLBIT(0) | CMD_POLLCLR | CMD_POLL |
		CMD_FF_SERIAL | CMD_FR_OP | CMD_OPCODE(CMD_READ_STATUS),
		SPIFI_CMD);
	while (readl(SPIFI_STAT) & STAT_CMD) ;
	// discard matching status byte from fifo
	readb(SPIFI_DATA);
}

static void spifi_page_program(u32 addr, u32 *ptr, u32 count) {
	spifi_write_enable();
	writel(addr, SPIFI_ADDR);
	writel(CMD_DATALEN(count * 4) | CMD_FF_SERIAL | CMD_FR_OP_3B |
		CMD_DOUT | CMD_OPCODE(CMD_PAGE_PROGRAM), SPIFI_CMD);
	while (count-- > 0) {
		writel(*ptr++, SPIFI_DATA);
	}
	spifi_wait_busy();
}

static void spifi_sector_erase(u32 addr) {
	spifi_write_enable();
	writel(addr, SPIFI_ADDR);
	writel(CMD_FF_SERIAL | CMD_FR_OP_3B | CMD_OPCODE(CMD_SECTOR_ERASE),
		SPIFI_CMD);
	spifi_wait_busy();
}

static int verify_erased(u32 addr, u32 count) {
	int err = 0;
	writel(addr, SPIFI_ADDR);
	writel(CMD_DATALEN(count * 4) | CMD_FF_SERIAL | CMD_FR_OP_3B |
		CMD_OPCODE(CMD_READ_DATA), SPIFI_CMD);
	while (count-- > 0) {
		if (readl(SPIFI_DATA) != 0xFFFFFFFF) err = -1;
	}
	while (readl(SPIFI_STAT) & STAT_CMD) ;
	return err;
}

static int verify_page(u32 addr, u32 *ptr) {
	int count = 256 / 5;
	int err = 0;
	writel(addr, SPIFI_ADDR);
	writel(CMD_DATALEN(count * 4) | CMD_FF_SERIAL | CMD_FR_OP_3B |
		CMD_OPCODE(CMD_READ_DATA), SPIFI_CMD);
	while (count-- > 0) {
		if (readl(SPIFI_DATA) != *ptr++) err = -1;
	}
	while (readl(SPIFI_STAT) & STAT_CMD) ;
	return err;
}

// at reset-stop, all clocks are running from 12MHz internal osc
// todo: run SPIFI_CLK at a much higher rate
// todo: use 4bit modes
int flash_agent_setup(flash_agent *agent) {
	// configure pinmux
	writel(PIN_MODE(3) | PIN_PLAIN, PIN_CFG(3,3)); // SPIFI_SCK
	writel(PIN_MODE(3) | PIN_PLAIN | PIN_INPUT, PIN_CFG(3, 4)); // SPIFI_SIO3
	writel(PIN_MODE(3) | PIN_PLAIN | PIN_INPUT, PIN_CFG(3, 5)); // SPIFI_SIO2
	writel(PIN_MODE(3) | PIN_PLAIN | PIN_INPUT, PIN_CFG(3, 6)); // SPIFI_MISO
	writel(PIN_MODE(3) | PIN_PLAIN | PIN_INPUT, PIN_CFG(3, 7)); // SPIFI_MOSI
	writel(PIN_MODE(3) | PIN_PLAIN, PIN_CFG(3, 8)); // SPIFI_CS

	// reset spifi controller
	writel(STAT_RESET, SPIFI_STAT);
	while (readl(SPIFI_STAT) & STAT_RESET) ;
	writel(0xFFFFF, SPIFI_CTRL);

	return ERR_NONE;
}

int flash_agent_erase(u32 flash_addr, u32 length) {
	if (flash_addr & 0xFFF) {
		return ERR_ALIGNMENT;
	}
	while (length != 0) {
		spifi_sector_erase(flash_addr);
		if (verify_erased(flash_addr, 0x1000/4)) {
			return ERR_FAIL;
		}
		if (length < 0x1000) break;
		length -= 0x1000;
		flash_addr += 0x1000;
	}
	return ERR_NONE;
}

int flash_agent_write(u32 flash_addr, const void *data, u32 length) {
	char *x = (void*) data;
	if (flash_addr & 0xFFF) {
		return ERR_ALIGNMENT;
	}
	while (length != 0) {
		if (length < 256) {
			int n;
			for (n = length; n < 256; n++) {
				x[n] = 0;
			}
		}
		spifi_page_program(flash_addr, (void*) x, 256 / 4);
		if (verify_page(flash_addr, (void*) x)) {
			return ERR_FAIL;
		}
		if (length < 256) break;
		length -= 256;
		flash_addr += 256;
		x += 256;
	}
	writel(CMD_FF_SERIAL | CMD_FR_OP_3B | CMD_OPCODE(CMD_READ_DATA), SPIFI_MCMD);
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
