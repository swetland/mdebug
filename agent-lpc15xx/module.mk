
M_NAME := agent-lpc15xx
M_CHIP := lpc1547-agt
M_START := agent-lpc15xx/header.o
M_OBJS := agent-lpc15xx/main.o
$(call build-target-executable)

