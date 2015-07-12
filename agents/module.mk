
M_NAME := agent-lpc15xx
M_CONFIG := ARCH_LPC15xx=1
M_ROMBASE := 0x00000000
M_ROMSIZE := 0x00010000
M_RAMBASE := 0x02000400
M_RAMSIZE := 0x00000400
M_OBJS := agents/lpc13xx_lpc15xx.o
$(call build-target-agent)

M_NAME := agent-lpc13xx
M_ROMBASE := 0x00000000
M_ROMSIZE := 0x00008000
M_RAMBASE := 0x10000400
M_RAMSIZE := 0x00000400
M_OBJS := agents/lpc13xx_lpc15xx.o
$(call build-target-agent)

M_NAME := agent-stm32f4xx
M_ROMBASE := 0x00000000
M_ROMSIZE := 0x00100000
M_RAMBASE := 0x20000400
M_RAMSIZE := 0x00000400
M_OBJS := agents/stm32fxxx.o
$(call build-target-agent)

