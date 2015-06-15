
#           name          arch      rambase    ramsize    flashbase  flashsize  linkscript
$(call chip,stm32f4xx-rom,stm32f1xx,0x20000000,0x00020000,0x00000000,0x00100000,rom)
$(call chip,stm32f4xx-ram,stm32f1xx,0x20000000,0x00020000,0x00000000,0x00000000,ram)
$(call chip,stm32f4xx-agt,stm32f1xx,0x20000400,0x0001fc00,0x00000000,0x00100000,ram)


ARCH_stm32f4xx_CFLAGS := -Iarch/stm32f4xx/include
ARCH_stm32f4xx_CFLAGS += -Iarch/arm-cm3/include
ARCH_stm32f4xx_CFLAGS += -DCONFIG_STACKTOP=0x20005000
ARCH_stm32f4xx_START := arch/arm-cm3/start.o

ARCH_stm32f4xx_OBJS :=
