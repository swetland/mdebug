#include <fw/types.h>

#include <arch/cpu.h>
#include <arch/iap.h>

#define IAP_OP_PREPARE		50
#define IAP_OP_WRITE		51
#define IAP_OP_ERASE_SECTOR	52
#define IAP_OP_BLANK_CHECK	53
#define IAP_OP_READ_PART_ID	54
#define IAP_OP_READ_BOOT_VERS	55
#define IAP_OP_COMPARE		56
#define IAP_OP_REINVOKE_ISP	57
#define IAP_OP_READ_UID		58
#define IAP_OP_ERASE_PAGE	59
#define IAP_OP_EEPROM_WRITE	61
#define IAP_OP_EEPROM_READ	62

void (*romcall)(u32 *in, u32 *out) = (void*) 0x1fff1ff1;

static int iap_eeprom_op(u32 op, void *buf, unsigned addr, int len)
{
	u32 in[5];
	u32 out[4];

	in[0] = op;
	in[1] = addr;
	in[2] = (u32) buf;
	in[3] = len;
	in[4] = 48000;

	disable_interrupts();
	romcall(in, out);
	enable_interrupts();

	return -((int)out[0]);
}

int iap_eeprom_read(void *buf, unsigned addr, int len)
{
	return iap_eeprom_op(IAP_OP_EEPROM_READ, buf, addr, len);
}

int iap_eeprom_write(unsigned addr, void *buf, int len)
{
	return iap_eeprom_op(IAP_OP_EEPROM_WRITE, buf, addr, len);
}

