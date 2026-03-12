#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# 3ds_rules
export DEVKITARM	?= /opt/devkitpro/devkitARM
export DEVKITPRO	?= /opt/devkitpro

PORTLIBS	:=	$(DEVKITPRO)/portlibs/3ds

CTRULIB	?=	$(DEVKITPRO)/libctru

export PATH := $(DEVKITPRO)/portlibs/3ds/bin:$(PATH)

# base_rules
export SHELL := /usr/bin/env bash

DEVKITPATH=$(shell echo "$(DEVKITPRO)" | sed -e 's/^\([a-zA-Z]\):/\/\1/')

export PATH := $(DEVKITPATH)/tools/bin:$(DEVKITPATH)/devkitARM/bin:$(PATH)

# 3DSX
VERSION_MAJOR	:=	1
VERSION_MINOR	:=	0
VERSION_MICRO	:=	0

APP_TITLE 	:= rockbox
APP_DESCRIPTION	:= Open Source Jukebox Firmware
APP_AUTHOR	:= rockbox.org
APP_ICON	:= $(ROOTDIR)/packaging/ctru/res/icon.png

# CIA
BANNER_AUDIO	:= $(ROOTDIR)/packaging/ctru/res/banner.wav
BANNER_IMAGE	:= $(ROOTDIR)/packaging/ctru/res/banner.cgfx
RSF_PATH	:= $(ROOTDIR)/packaging/ctru/res/app.rsf
#LOGO		:= $(ROOTDIR)/packaging/ctru/logo.lz11
UNIQUE_ID	:= 0xCB001
PRODUCT_CODE	:= CTR-ROCKBOX
ICON_FLAGS	:= nosavebackups,visible

# CIA Configuration
USE_ON_SD       := true
APP_ENCRYPTED   := false
CATEGORY        := Application
USE_ON_SD       := true
MEMORY_TYPE     := Application
SYSTEM_MODE     := 64MB
SYSTEM_MODE_EXT := Legacy
CPU_SPEED       := 268MHz
ENABLE_L2_CACHE := false

.SECONDEXPANSION: # $$(OBJ) is not populated until after this

SMDH_PATH   := $(BUILDDIR)/rockbox.smdh
3DSX_PATH   := $(BUILDDIR)/rockbox.3dsx

BANNER_PATH := $(BUILDDIR)/rockbox.bnr
ICON_PATH   := $(BUILDDIR)/rockbox.icn
CIA_PATH    := $(BUILDDIR)/rockbox.cia

SMDHTOOL ?= smdhtool
3DSXTOOL ?= 3dsxtool

BANNERTOOL   ?= bannertool

ifeq ($(suffix $(BANNER_IMAGE)),.cgfx)
	BANNER_IMAGE_ARG := -ci
else
	BANNER_IMAGE_ARG := -i
endif

ifeq ($(suffix $(BANNER_AUDIO)),.cwav)
	BANNER_AUDIO_ARG := -ca
else
	BANNER_AUDIO_ARG := -a
endif

MAKEROM      ?= makerom
MAKEROM_ARGS := -DAPP_TITLE="$(APP_TITLE)" -DAPP_PRODUCT_CODE="$(PRODUCT_CODE)" -DAPP_UNIQUE_ID="$(UNIQUE_ID)" \
                -DAPP_ENCRYPTED="$(APP_ENCRYPTED)" -DAPP_SYSTEM_MODE="$(SYSTEM_MODE)" \
                -DAPP_SYSTEM_MODE_EXT="$(SYSTEM_MODE_EXT)" -DAPP_CATEGORY="$(CATEGORY)" -DAPP_USE_ON_SD="$(USE_ON_SD)" \
                -DAPP_MEMORY_TYPE="$(MEMORY_TYPE)" -DAPP_CPU_SPEED="$(CPU_SPEED)" \
                -DAPP_ENABLE_L2_CACHE="$(ENABLE_L2_CACHE)" \
                -major $(VERSION_MAJOR) -minor $(VERSION_MINOR) -micro $(VERSION_MICRO)

ifneq ($(strip $(LOGO)),)
	MAKEROM_ARGS	+= -logo "$(LOGO)"
endif
ifneq ($(strip $(ROMFS)),)
	MAKEROM_ARGS	+= -DAPP_ROMFS="$(ROMFS)"
endif

$(BUILDDIR)/$(BINARY): $$(OBJ) $(FIRMLIB) $(VOICESPEEXLIB) $(CORE_LIBS)
ifeq ($(UNAME), Darwin)
	$(call PRINTS,LD $(@F))$(CC) -o $@ $^ $(LDOPTS) $(GLOBAL_LDOPTS) -Wl,$(LDMAP_OPT),$(BUILDDIR)/rockbox.map
else
	$(call PRINTS,LD $(@F))$(CC) -o $@ -Wl,--start-group $^ -Wl,--end-group $(LDOPTS) $(GLOBAL_LDOPTS) \
	-Wl,$(LDMAP_OPT),$(BUILDDIR)/rockbox.map
endif

$(SMDH_PATH): $(APP_ICON)
	$(SMDHTOOL) --create "$(APP_TITLE)" "$(APP_DESCRIPTION)" "$(APP_AUTHOR)" $< $@

$(3DSX_PATH): $(BUILDDIR)/$(BINARY) $(SMDH_PATH)
	$(3DSXTOOL) $< $@ --smdh=$(SMDH_PATH)

$(BANNER_PATH): $(BANNER_IMAGE) $(BANNER_AUDIO)
	$(BANNERTOOL) makebanner $(BANNER_IMAGE_ARG) $(BANNER_IMAGE) $(BANNER_AUDIO_ARG) $(BANNER_AUDIO) -o $@

$(ICON_PATH): $(APP_ICON)
	$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" -l "$(APP_DESCRIPTION)" -p "$(APP_AUTHOR)" -i $< -f $(ICON_FLAGS) -o $@

$(CIA_PATH): $(BUILDDIR)/$(BINARY) $(RSF_PATH) $(BANNER_PATH) $(ICON_PATH)
	$(MAKEROM) -f cia -o $@ -target t -exefslogo -elf $< -rsf $(RSF_PATH) -banner $(BANNER_PATH) \
	-icon $(ICON_PATH) $(MAKEROM_ARGS)

# add dependencies to build the packages
CTRU_PACKAGES := $(3DSX_PATH) $(CIA_PATH)

build: $(CTRU_PACKAGES)
bin: $(CTRU_PACKAGES)
