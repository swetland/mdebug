
#           name        arch       rambase    ramsize    flashbase  flashsize  linkscript
$(call chip,lpc1342-ram,lpc13xx_v1,0x10000000,0x00001000,0x00000000,0x00000000,ram)
$(call chip,lpc1342-rom,lpc13xx_v1,0x10000000,0x00001000,0x00000000,0x00004000,rom)

$(call chip,lpc1343-ram,lpc13xx_v1,0x10000000,0x00002000,0x00000000,0x00000000,ram)
$(call chip,lpc1343-rom,lpc13xx_v1,0x10000000,0x00002000,0x00000000,0x00008000,rom)
$(call chip,lpc1343-blr,lpc13xx_v1,0x10001c00,0x00000400,0x00000000,0x00001000,rom)
$(call chip,lpc1343-app,lpc13xx_v1,0x10000000,0x00002000,0x00001000,0x00007000,rom)

$(call chip,lpc1345-rom,lpc13xx_v2,0x10000000,0x00002000,0x00000000,0x00008000,rom)
$(call chip,lpc1347-rom,lpc13xx_v2,0x10000000,0x00002000,0x00000000,0x00008000,rom)


ARCH_lpc13xx_v1_CFLAGS := -Iarch/lpc13xx/include
ARCH_lpc13xx_v1_CFLAGS += -Iarch/lpc13xx/include-v1
ARCH_lpc13xx_v1_CFLAGS += -Iarch/arm-cm3/include
ARCH_lpc13xx_v1_CFLAGS += -DCONFIG_STACKTOP=0x10001f00
ARCH_lpc13xx_v1_START := arch/arm-cm3/start.o
ARCH_lpc13xx_v1_CONFIG := LPC13XX_V1=1

ARCH_lpc13xx_v2_CFLAGS := -Iarch/lpc13xx/include
ARCH_lpc13xx_v2_CFLAGS += -Iarch/lpc13xx/include-v2
ARCH_lpc13xx_v2_CFLAGS += -Iarch/arm-cm3/include
ARCH_lpc13xx_v2_CFLAGS += -DCONFIG_STACKTOP=0x10001f00
ARCH_lpc13xx_v2_START := arch/arm-cm3/start.o
ARCH_lpc13xx_v2_CONFIG := LPC13XX_V2=1

ARCH_lpc13xx_OBJS := \
	arch/lpc13xx/init.o \
	arch/lpc13xx/iap.o \
	arch/lpc13xx/reboot.o \
	arch/lpc13xx/serial.o

ARCH_lpc13xx_v1_OBJS := \
	$(ARCH_lpc13xx_OBJS) \
	arch/lpc13xx/gpio-v1.o \
	arch/lpc13xx/usb-v1.o

ARCH_lpc13xx_v2_OBJS := \
	$(ARCH_lpc13xx_OBJS) \
	arch/lpc13xx/gpio-v2.o \
	arch/lpc13xx/usb-v2.o
