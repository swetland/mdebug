
#           name        arch    rambase    ramsize    flashbase  flashsize  linkscript
$(call chip,lpc1549-ram,lpc15xx,0x02000000,0x00009000,0x00000000,0x00000000,ram)
$(call chip,lpc1549-rom,lpc15xx,0x02000000,0x00009000,0x00000000,0x00040000,rom)
$(call chip,lpc1549-blr,lpc15xx,0x02008c00,0x00000400,0x00000000,0x00001000,rom)
$(call chip,lpc1549-app,lpc15xx,0x02000000,0x00009000,0x00001000,0x0003F000,rom)

$(call chip,lpc1548-ram,lpc15xx,0x02000000,0x00005000,0x00000000,0x00000000,ram)
$(call chip,lpc1548-rom,lpc15xx,0x02000000,0x00005000,0x00000000,0x00020000,rom)
$(call chip,lpc1548-blr,lpc15xx,0x02004c00,0x00000400,0x00000000,0x00001000,rom)
$(call chip,lpc1548-app,lpc15xx,0x02000000,0x00005000,0x00001000,0x0001F000,rom)

$(call chip,lpc1547-ram,lpc15xx,0x02000000,0x00003000,0x00000000,0x00000000,ram)
$(call chip,lpc1547-rom,lpc15xx,0x02000000,0x00003000,0x00000000,0x00010000,rom)
$(call chip,lpc1547-blr,lpc15xx,0x02002c00,0x00000400,0x00000000,0x00001000,rom)
$(call chip,lpc1547-app,lpc15xx,0x02000000,0x00003000,0x00001000,0x0000F000,rom)

ARCH_lpc15xx_CFLAGS := \
	-Iarch/lpc15xx/include \
	-Iarch/arm-cm3/include 
ARCH_lpc15xx_START := arch/arm-cm3/start.o
ARCH_lpc15xx_CONFIG := \
	ARCH_LPC15XX=1 \
	STACKTOP=0x02008f00
ARCH_lpc15xx_OBJS := \
	arch/lpc15xx/gpio.o

