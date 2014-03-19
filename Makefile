## Copyright 2011 Brian Swetland <swetland@frotz.net>
## 
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##     http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.

what_to_build:: all

-include local.mk

TOOLCHAIN ?= arm-none-eabi-

TARGET_CC := $(TOOLCHAIN)gcc
TARGET_LD := $(TOOLCHAIN)ld
TARGET_AR := $(TOOLCHAIN)ar
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
TARGET_LFLAGS := --gc-sections

#TARGET_LIBGCC := $(shell $(TARGET_CC) $(TARGET_CFLAGS) -print-libgcc-file-name)

UNAME := $(shell uname)
UNAME_M := $(shell uname -m)

HOST_CFLAGS := -g -O1 -Wall
HOST_CFLAGS += -Itools -Iinclude

ifeq ($(UNAME),Darwin)
HOST_CFLAGS += -I/opt/local/include -L/opt/local/lib
HOST_LIBS += -lusb-1.0
endif
ifeq ($(UNAME),Linux)
HOST_CFLAGS += -DWITH_THUMB2_DISASSEMBLE=1
HOST_LIBS += -lusb-1.0
endif

OUT := out
BIN := bin
OUT_HOST_OBJ := $(OUT)/_obj/host
OUT_TARGET_OBJ := $(OUT)/_obj/target

ALL :=
DEPS :=

# build system internals
include build/build.mk

# architecture configurations
include $(wildcard arch/*/config.mk)

# additional modules
include $(wildcard */module.mk)

clean::
	@echo clean
	@rm -rf $(OUT) $(BIN)

# we generate .d as a side-effect of compiling. override generic rule:
%.d:
-include $(DEPS)

all:: $(ALL)

