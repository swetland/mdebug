$(call start-module-mk)

M_NAME := m3debug 
M_CHIP := lpc1343-app
M_OBJS += board/m3debug.o
M_OBJS += m3debug/main.o
M_OBJS += m3debug/swdp.o
M_OBJS += libfw/print.o
M_OBJS += libfw/serialconsole.o
$(call build-target-executable)

