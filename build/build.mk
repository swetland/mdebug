## Copyright 2014 Brian Swetland <swetland@frotz.net>
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

# configuration header generation heavily inspired by travisg's lk build system

# $(call chip,name,arch,rambase,ramsize,rombase,romsize,linkscript)
define chip
$(eval CHIP_$1_ARCH := $2) \
$(eval CHIP_$1_RAMBASE := $3) \
$(eval CHIP_$1_RAMSIZE := $4) \
$(eval CHIP_$1_ROMBASE := $5) \
$(eval CHIP_$1_ROMSIZE := $6) \
$(eval CHIP_$1_LINKSCRIPT := build/generic-$7.ld) \
$(eval CHIP_$1_DEPS := $(lastword $(MAKEFILE_LIST)))
endef

MKDIR = if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi

QUIET ?= @

SPACE :=
SPACE +=
COMMA := ,

define underscorify
$(subst /,_,$(subst \,_,$(subst .,_,$(subst -,_,$1))))
endef

define toupper
$(subst a,A,$(subst b,B,$(subst c,C,$(subst d,D,$(subst e,E,$(subst f,F,$(subst g,G,$(subst h,H,$(subst i,I,$(subst j,J,$(subst k,K,$(subst l,L,$(subst m,M,$(subst n,N,$(subst o,O,$(subst p,P,$(subst q,Q,$(subst r,R,$(subst s,S,$(subst t,T,$(subst u,U,$(subst v,V,$(subst w,W,$(subst x,X,$(subst y,Y,$(subst z,Z,$1))))))))))))))))))))))))))
endef

# (call make-config-header,outfile,configlist)
define make-config-header
echo "/* Machine Generated File - Do Not Edit */" >> $1.tmp ; \
echo "#ifndef __$(call underscorify,$1)" >> $1.tmp ; \
echo "#define __$(call underscorify,$1)" >> $1.tmp ; \
$(foreach def,$2,echo "#define CONFIG_$(subst =, ,$(call underscorify,$(call toupper,$(def))))" >> $1.tmp ;) \
echo "#endif" >> $1.tmp ; \
mv $1.tmp $1
endef

start-module-mk = $(eval M_MAKEFILE := $(lastword $(MAKEFILE_LIST)))
build-target-executable = $(eval include build/target-executable.mk)
build-host-executable = $(eval include build/host-executable.mk)

