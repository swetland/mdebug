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

# sanity check
ifeq "$(M_NAME)" ""
$(error No module name specified)
endif

M_OBJS := $(addprefix $(OUT_HOST_OBJ)/$(M_NAME)/,$(M_OBJS))
DEPS += $(M_OBJS:%o=%d)

ALL += $(BIN)/$(M_NAME)

$(OUT_HOST_OBJ)/$(M_NAME)/%.o: %.c
	@$(MKDIR)
	@echo compile $<
	$(QUIET)$(CC) $(HOST_CFLAGS) -c $< -o $@ -MD -MT $@ -MF $(@:%o=%d)

$(BIN)/$(M_NAME): _OBJS := $(M_OBJS)
$(BIN)/$(M_NAME): _LIBS := $(M_LIBS) $(HOST_LIBS)
$(BIN)/$(M_NAME): $(M_OBJS)
	@$(MKDIR)
	@echo link $@
	$(QUIET)gcc $(HOST_CFLAGS) -o $@ $(_OBJS) $(_LIBS)

$(info module $(M_NAME))

M_LIBS :=
M_OBJS :=
M_NAME :=
