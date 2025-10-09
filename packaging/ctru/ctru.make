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

MAKEROM      ?= makerom
MAKEROM_ARGS := -elf "$(BINARY).elf" -rsf "$(RSF_PATH)" -banner "$(BUILDDIR)/banner.bnr" -icon "$(BUILDDIR)/icon.icn" -DAPP_TITLE="$(APP_TITLE)" -DAPP_PRODUCT_CODE="$(PRODUCT_CODE)" -DAPP_UNIQUE_ID="$(UNIQUE_ID)" -DAPP_ENCRYPTED="$(APP_ENCRYPTED)" -DAPP_SYSTEM_MODE="$(SYSTEM_MODE)" -DAPP_SYSTEM_MODE_EXT="$(SYSTEM_MODE_EXT)" -DAPP_CATEGORY="$(CATEGORY)" -DAPP_USE_ON_SD="$(USE_ON_SD)" -DAPP_MEMORY_TYPE="$(MEMORY_TYPE)" -DAPP_CPU_SPEED="$(CPU_SPEED)" -DAPP_ENABLE_L2_CACHE="$(ENABLE_L2_CACHE)"
MAKEROM_ARGS += -major $(VERSION_MAJOR) -minor $(VERSION_MINOR) -micro $(VERSION_MICRO)

ifneq ($(strip $(LOGO)),)
	MAKEROM_ARGS	+= -logo "$(LOGO)"
endif
ifneq ($(strip $(ROMFS)),)
	MAKEROM_ARGS	+= -DAPP_ROMFS="$(ROMFS)"
endif

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

# main binary
$(BUILDDIR)/$(BINARY): $$(OBJ) $(FIRMLIB) $(VOICESPEEXLIB) $(CORE_LIBS)
ifeq ($(UNAME), Darwin)
	$(call PRINTS,LD $(BINARY))$(CC) -o $@ $^ $(LDOPTS) $(GLOBAL_LDOPTS) -Wl,$(LDMAP_OPT),$(BUILDDIR)/rockbox.map
else
	$(call PRINTS,LD $(BINARY))$(CC) -o $@ -Wl,--start-group $^ -Wl,--end-group $(LDOPTS) $(GLOBAL_LDOPTS) \
	-Wl,$(LDMAP_OPT),$(BUILDDIR)/rockbox-.map
	@mv $(BINARY) $(BINARY).elf
	smdhtool --create "$(APP_TITLE)" "$(APP_DESCRIPTION)" "$(APP_AUTHOR)" $(APP_ICON) "rockbox.smdh"
	3dsxtool $(BINARY).elf $(BINARY).3dsx --smdh="rockbox.smdh"
	$(BANNERTOOL) makebanner $(BANNER_IMAGE_ARG) "$(BANNER_IMAGE)" $(BANNER_AUDIO_ARG) "$(BANNER_AUDIO)" -o "$(BUILDDIR)/banner.bnr"
	$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" -l "$(APP_DESCRIPTION)" -p "$(APP_AUTHOR)" -i "$(APP_ICON)" -f "$(ICON_FLAGS)" -o "$(BUILDDIR)/icon.icn"
	$(MAKEROM) -f cia -o "$(BINARY).cia" -target t -exefslogo $(MAKEROM_ARGS)
ifndef DEBUG
	$(SILENT)rm $(BINARY).elf
endif
endif

