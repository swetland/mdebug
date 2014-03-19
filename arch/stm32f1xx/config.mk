
#           name          arch      rambase    ramsize    flashbase  flashsize  linkscript
$(call chip,stm32f103-rom,stm32f1xx,0x20000000,0x00005000,0x08000000,0x00020000,rom)


ARCH_stm32f1xx_CFLAGS := -Iarch/stm32f1xx/include
ARCH_stm32f1xx_CFLAGS += -Iarch/arm-cm3/include
ARCH_stm32f1xx_CFLAGS += -DCONFIG_STACKTOP=0x20005000
ARCH_stm32f1xx_START := arch/arm-cm3/start.o

ARCH_stm32f1xx_OBJS := \
	arch/stm32f1xx/gpio.o \
	arch/stm32f1xx/serial.o \
	arch/stm32f1xx/usb.o
