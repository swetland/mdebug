/* unused.c
 *
 * Copyright 2011 Brian Swetland <swetland@frotz.net>
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* tidbits from when the device side was smarter */
/* may be useful again someday */

static unsigned prevaddr;

static unsigned char ap_rr[4] = { RD_AP0, RD_AP1, RD_AP2, RD_AP3 };
static unsigned char ap_wr[4] = { WR_AP0, WR_AP1, WR_AP2, WR_AP3 };

unsigned swdp_ap_read(unsigned addr) {
	if ((addr & 0xF0) != prevaddr) {
		swdp_write(WR_SELECT, (addr & 0xF0) | (0 << 24));
		prevaddr = addr & 0xF0;
	}
	swdp_read(ap_rr[(addr >> 2) & 3]);
	return swdp_read(RD_BUFFER);
}

unsigned swdp_ap_write(unsigned addr, unsigned value) {
	if ((addr & 0xF0) != prevaddr) {
		swdp_write(WR_SELECT, (addr & 0xF0) | (0 << 24));
		prevaddr = addr & 0xF0;
	}
	swdp_write(ap_wr[(addr >> 2) & 3], value);
}

void crap(void) {
	/* invalidate AP address cache */
	prevaddr = 0xFF;

	printx("IDCODE= %x\n", idcode);

	swdp_write(WR_ABORT, 0x1E); // clear any stale errors
	swdp_write(WR_DPCTRL, (1 << 28) | (1 << 30)); // POWER UP
	n = swdp_read(RD_DPCTRL);
	printx("DPCTRL= %x\n", n);

	/* configure for 32bit IO */
	swdp_ap_write(AHB_CSW, AHB_CSW_MDEBUG | AHB_CSW_PRIV |
		AHB_CSW_PRIV | AHB_CSW_DBG_EN | AHB_CSW_32BIT);

	printx("ROMADDR= %x", swdp_ap_read(AHB_ROM_ADDR));
}

unsigned swdp_core_read(unsigned n) {
	swdp_ahb_write(CDBG_REG_ADDR, n);
	return swdp_ahb_read(CDBG_REG_DATA);
}

void swdp_core_halt(void) {
	unsigned n = swdp_ahb_read(CDBG_CSR);
	swdp_ahb_write(CDBG_CSR, 
		CDBG_CSR_KEY | (n & 0x3F) |
		CDBG_C_HALT | CDBG_C_DEBUGEN);
}

void swdp_core_resume(void) {
	unsigned n = swdp_ahb_read(CDBG_CSR);
	swdp_ahb_write(CDBG_CSR,
		CDBG_CSR_KEY | (n & 0x3C));
}

unsigned swdp_ahb_read(unsigned addr) {
	swdp_ap_write(AHB_TAR, addr);
	return swdp_ap_read(AHB_DRW);
}

void swdp_ahb_write(unsigned addr, unsigned value) {
	swdp_ap_write(AHB_TAR, addr);
	swdp_ap_write(AHB_DRW, value);
}

void swdp_test(void) {
	unsigned n;
	swdp_reset();


	for (n = 0x20000000; n < 0x20000020; n += 4) {
		swdp_ap_write(AHB_TAR, n);
		printx("%x: %x\n", n, swdp_ap_read(AHB_DRW));
	}

	swdp_ap_write(AHB_TAR, 0xE000EDF0);
	swdp_ap_write(AHB_DRW, 0xA05F0003);

	swdp_ap_write(AHB_TAR, 0x20000000);
	swdp_ap_write(AHB_DRW, 0x12345678);
	printx("DPCTRL= %x\n", swdp_read(RD_DPCTRL));
	printx("DPCTRL= %x\n", swdp_read(RD_DPCTRL));
	swdp_read(RD_BUFFER);
	swdp_read(RD_BUFFER);
}

