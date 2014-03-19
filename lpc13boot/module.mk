$(call start-module-mk)

M_NAME := lpc13boot
M_CHIP := lpc1343-blr
M_OBJS := board/m3debug.o
M_OBJS += lpc13boot/main.o
M_OBJS += lpc13boot/misc.o
M_OBJS += libc/strcpy.o
M_SIGN := bin/bless-lpc
$(call build-target-executable)

