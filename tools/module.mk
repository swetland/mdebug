$(call start-module-mk)

M_NAME := debugger
M_OBJS := tools/debugger.o
M_OBJS += tools/debugger-core.o
M_OBJS += tools/debugger-commands.o
M_OBJS += tools/rswdp.o
M_OBJS += tools/linenoise.o
M_OBJS += tools/usb.o
M_OBJS += tools/socket.o
M_OBJS += tools/gdb-bridge.o
M_OBJS += tools/lkdebug.o
M_OBJS += out/debugger-builtins.o
$(call build-host-executable)

out/debugger-builtins.c: $(AGENTS) bin/mkbuiltins
	@mkdir -p out
	./bin/mkbuiltins $(AGENTS) > $@	

M_NAME := stm32boot
M_OBJS := tools/stm32boot.o
$(call build-host-executable)

ifeq ($(UNAME),Linux)
M_NAME := usbmon
M_OBJS := tools/usbmon.o
$(call build-host-executable)
endif

M_NAME := usbtest
M_OBJS := tools/usbtest.o tools/usb.o
$(call build-host-executable)

M_NAME := bless-lpc
M_OBJS := tools/bless-lpc.o
$(call build-host-executable)

M_NAME := lpcboot
M_OBJS := tools/lpcboot.o tools/usb.o
$(call build-host-executable)

M_NAME := uconsole 
M_OBJS := tools/uconsole.o tools/usb.o
$(call build-host-executable)

M_NAME := mkbuiltins
M_OBJS := tools/mkbuiltins.o
$(call build-host-executable)
