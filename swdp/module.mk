$(call start-module-mk)

M_NAME := swdp
M_CHIP := stm32f103-rom
M_OBJS := swdp/main.o
M_OBJS += swdp/swdp.o
M_OBJS += libfw/print.o
M_OBJS += libfw/serialconsole.o
$(call build-target-executable)
