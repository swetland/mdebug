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

M_NAME := $(strip $(M_NAME))
M_CHIP := $(strip $(M_CHIP))

ifeq ($(strip $(M_START)),)
M_START := $(ARCH_$(M_ARCH)_START) 
endif

M_ARCH := $(CHIP_$(M_CHIP)_ARCH)
M_ARCH_CFLAGS := $(ARCH_$(M_ARCH)_CFLAGS)
M_ARCH_OBJS := $(ARCH_$(M_ARCH)_OBJS)

# sanity check
ifeq "$(M_NAME)" ""
$(error $(M_MAKEFILE): No module name specified)
endif

# architecture start glue goes first
M_OBJS := $(addprefix $(OUT_TARGET_OBJ)/$(M_NAME)/,$(M_OBJS))

DEPS += $(M_OBJS:%o=%d)

M_OUT_BIN := $(OUT)/$(M_NAME).bin
M_OUT_LST := $(OUT)/$(M_NAME).lst
M_OUT_ELF := $(OUT)/$(M_NAME).elf

ALL += $(M_OUT_BIN) $(M_OUT_LST) $(M_OUT_ELF)

M_INCLUDE := $(OUT_TARGET_OBJ)/$(M_NAME)/include
M_CONFIG_H := $(M_INCLUDE)/config.h
M_LINK_SCRIPT := $(OUT_TARGET_OBJ)/$(M_NAME)/script.ld

# generate link script 
$(M_LINK_SCRIPT): _RADDR := $(M_RAMBASE)
$(M_LINK_SCRIPT): _RSIZE := $(M_RAMSIZE)
$(M_LINK_SCRIPT): $(CHIP_$(M_CHIP)_DEPS)
	@echo linkscript $@
	@echo "MEMORY {" > $@
	@echo " RAM (xrw) : ORIGIN = $(_RADDR), LENGTH = $(_RSIZE)" >> $@
	@echo "}" >> $@
	@echo " INCLUDE \"build/generic-ram.ld\"" >> $@

$(OUT_TARGET_OBJ)/$(M_NAME)/%.o: %.c
	@$(MKDIR)
	@echo compile $<
	$(QUIET)$(TARGET_CC) $(TARGET_CFLAGS) $(_CFLAGS) -c $< -o $@ -MD -MT $@ -MF $(@:%o=%d)

$(OUT_TARGET_OBJ)/$(M_NAME)/%.o: %.S
	@$(MKDIR)
	@echo assemble $<
	$(QUIET)$(TARGET_CC) $(TARGET_CFLAGS) $(_CFLAGS) -c $< -o $@ -MD -MT $@ -MF $(@:%o=%d)

# apply our flags to our objects
$(M_OBJS): _CFLAGS := --include $(M_CONFIG_H) $(M_CFLAGS)
$(M_ARCH_OBJS): _CFLAGS := --include $(M_CONFIG_H) $(M_CFLAGS)

# objects depend on generated config header
$(M_OBJS): $(M_CONFIG_H)
$(M_ARCH_OBJS): $(M_CONFIG_H)

X_CONFIG := FLASHSIZE=$(M_ROMSIZE)
X_CONFIG += FLASHADDR=$(M_ROMBASE)
X_CONFIG += LOADADDR=$(M_RAMBASE)

# generate config header from module, chip, and arch config lists
# generated config header depends on toplevel, module, and chip/arch makefiles
$(M_CONFIG_H): _CFG := $(M_CONFIG) $(X_CONFIG)
$(M_CONFIG_H): Makefile $(M_MAKEFILE)
	@$(MKDIR)
	@echo generate $@
	@$(call make-config-header,$@,$(_CFG))

$(M_OUT_BIN): $(M_OUT_ELF)
	@echo create $@
	$(QUIET)$(TARGET_OBJCOPY) --gap-fill=0xee -O binary $< $@

$(M_OUT_LST): $(M_OUT_ELF)
	@echo create $@
	$(QUIET)$(TARGET_OBJDUMP) --source -d $< > $@

$(M_OUT_ELF): _OBJS := $(M_OBJS)
$(M_OUT_ELF): _LINK := $(M_LINK_SCRIPT)
$(M_OUT_ELF): $(M_OBJS) $(M_LINK_SCRIPT)
	@echo link $@
	$(QUIET)$(TARGET_LD) $(TARGET_LFLAGS) -Bstatic -T $(_LINK) $(_OBJS) $(_LIBS) -o $@

AGENTS += $(M_OUT_BIN)

$(info module $(M_NAME))

M_START :=
M_OBJS :=
M_NAME :=
M_BASE :=
M_LIBS :=
M_CFLAGS :=
M_CONFIG :=
M_SIGN :=
M_RAMBASE :=
M_RAMSIZE :=
M_ROMBASE :=
M_ROMSIZE :=
