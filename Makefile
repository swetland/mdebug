
what_to_build:: all

-include local.mk

include build/build.mk

# bootloader download tool
SRCS := tools/lpcboot.c \
	tools/usb.c
$(call program,lpcboot,$(SRCS))

# debugger
SRCS := tools/debugger.c \
	tools/debugger-core.c \
	tools/debugger-commands.c \
	tools/gdb-bridge.c \
	tools/linenoise.c \
	tools/lkdebug.c \
	tools/rswdp.c \
	tools/socket.c \
	tools/swo.c \
	tools/usb.c

ifneq ($(TOOLCHAIN),)
SRCS += out/builtins.c
else
SRCS += tools/builtins.c
endif
$(call program,debugger,$(SRCS))


ifneq ($(TOOLCHAIN),)
# if there's a cross-compiler, build agents from source

$(call agent, lpclink2,  0x10080400)
$(call agent, stm32f4xx, 0x20000400)
$(call agent, lpc13xx,   0x10000400)
$(call agent, lpc15xx,   0x02000400)

# tool to pack the agents into a source file
SRCS := tools/mkbuiltins.c
$(call program,mkbuiltins,$(SRCS))

AGENT_BINS := $(patsubst %,out/agent-%.bin,$(AGENTS))

out/builtins.c: $(AGENT_BINS) bin/mkbuiltins
	@mkdir -p $(dir $@)
	@echo generate $@
	$(QUIET)./bin/mkbuiltins $(AGENT_BINS) > $@
endif

-include $(DEPS)

all: $(ALL)

clean::
	rm -rf out bin
