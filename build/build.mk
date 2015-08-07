
what_to_build:: all

-include local.mk

#TOOLCHAIN ?= arm-none-eabi-

TARGET_CC := $(TOOLCHAIN)gcc
TARGET_OBJCOPY := $(TOOLCHAIN)objcopy
TARGET_OBJDUMP := $(TOOLCHAIN)objdump

TARGET_CFLAGS := -g -Os -Wall
TARGET_CFLAGS += -Wno-unused-but-set-variable
TARGET_CFLAGS += -I. -Iinclude
TARGET_CFLAGS += -mcpu=cortex-m3 -mthumb -mthumb-interwork
TARGET_CFLAGS += -ffunction-sections -fdata-sections
TARGET_CFLAGS += -fno-builtin -nostdlib

# tell gcc there's not a full libc it can depend on
# so it won't do thinks like printf("...") -> puts("...")
TARGET_CFLAGS += -ffreestanding

QUIET := @

UNAME := $(shell uname)
UNAME_M := $(shell uname -m)

HOST_CFLAGS := -g -O1 -Wall
HOST_CFLAGS += -Itools -Iinclude
HOST_CFLAGS += -DLINENOISE_INTERRUPTIBLE

ifeq ($(UNAME),Darwin)
HOST_CFLAGS += -I/opt/local/include -L/opt/local/lib
HOST_LIBS += -lusb-1.0
endif
ifeq ($(UNAME),Linux)
HOST_LIBS += -lusb-1.0 -lpthread -lrt
endif

AGENTS :=
ALL :=
DEPS :=

out/agent-%.bin: out/agent-%.elf
	@mkdir -p $(dir $@)
	@echo generate $@
	$(QUIET)$(TARGET_OBJCOPY) -O binary $< $@

out/agent-%.lst: out/agent-%.elf
	@mkdir -p $(dir $@)
	@echo generate $@
	$(QUIET)$(TARGET_OBJDUMP) -d $< > $@

out/agent-%.elf: agents/%.c
	@mkdir -p $(dir $@)
	@echo compile $@
	$(QUIET)$(TARGET_CC) $(TARGET_CFLAGS) -Wl,--script=build/agent.ld -Wl,-Ttext=$(LOADADDR) -o $@ $<

out/%.o: %.c
	@mkdir -p $(dir $@)
	@echo compile $<
	$(QUIET)gcc -MMD -MP -c $(HOST_CFLAGS) -o $@ $< 

define _program
ALL += bin/$1
DEPS += $3
bin/$1: $2
	@mkdir -p $$(dir $$@)
	@echo link $$@
	$(QUIET)gcc $(HOST_CFLAGS) -o $$@ $2 $(HOST_LIBS)
endef

program = $(eval $(call _program,$1,$(patsubst %.c,out/%.o,$2),$(patsubst %.c,out/%.d,$2)))

agent = $(eval AGENTS += $(strip $1))\
$(eval ALL += $(patsubst %,out/agent-%.bin,$(strip $1)))\
$(eval ALL += $(patsubst %,out/agent-%.lst,$(strip $1)))\
$(eval out/agent-$(strip $1).elf: LOADADDR := $(strip $2))

