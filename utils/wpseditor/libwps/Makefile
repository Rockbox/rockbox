#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
ROOT=../../..

ifneq ($or($(findstring MINGW,$(shell uname)),$(findstring Windows_NT,$(OS))),)
  OS = w32
  CC = mingw32-gcc
  COPY = copy /Y
  COPY_TO = ..\\gui\\bin
  RM = del
  EXT = .dll
  LDFLAGS = -DBUILD_DLL
endif

ifeq ($(findstring Linux,$(shell uname)),Linux)
  OS = linux
  CC = gcc
  COPY = cp -f
  COPY_TO = ../gui/bin
  RM = rm -f
  EXT = .so
  LDFLAGS = -fPIC
endif

TARGETS=IRIVER_H10 \
        IRIVER_H10_5GB \
        IPOD_COLOR \
        IPOD_NANO \
        IPOD_VIDEO \
        IPOD_3G \
        IPOD_4G \
        IPOD_MINI \
        IPOD_MINI2G \
        IPOD_1G2G \
        GIGABEAT_F \
        GIGABEAT_S \
        SANSA_E200 \
        SANSA_C200 \
        IRIVER_H100 \
        IRIVER_H120 \
        IRIVER_H300 \
        IAUDIO_X5 \
        IAUDIO_M5 \
        IAUDIO_M3 \
        
SOURCES= \
        src/api.c \
        src/dummies.c \
        src/lcd.c \
        src/proxy.c \
        $(ROOT)/apps/gui/scrollbar.c \
        $(ROOT)/apps/gui/gwps-common.c \
        $(ROOT)/apps/gui/wps_parser.c \
        $(ROOT)/apps/gui/wps_debug.c \
        $(ROOT)/apps/recorder/peakmeter.c \
        $(ROOT)/apps/recorder/icons.c \
        $(ROOT)/apps/misc.c \
        $(ROOT)/apps/status.c \
        $(ROOT)/firmware/common/ctype.c \
        $(ROOT)/firmware/common/timefuncs.c \
        $(ROOT)/firmware/common/unicode.c \
        $(ROOT)/firmware/font.c \
        $(ROOT)/firmware/font_cache.c \
        $(ROOT)/firmware/id3.c \
        $(ROOT)/firmware/lru.c \
        $(ROOT)/firmware/mp3data.c \
        $(ROOT)/firmware/replaygain.c 
#        $(ROOT)/apps/recorder/bmp.c
#        $(ROOT)/apps/abrepeat.c \
#        $(ROOT)/apps/action.c \
#        $(ROOT)/apps/cuesheet.c \
#        $(ROOT)/apps/gui/statusbar.c \
#        $(ROOT)/apps/gui/gwps.c \

INCLUDE=-I src/include \
        -I $(ROOT)/apps/gui \
        -I $(ROOT)/firmware/export \
        -I $(ROOT)/firmware/include \
        -I $(ROOT)/apps/recorder \
        -I $(ROOT)/apps \
        -I src

CFLAGS = -g -Wall -Wno-format -D__PCTOOL__ -DWPSEDITOR -DDEBUG -DROCKBOX_DIR_LEN=1 -DBUTTON_REMOTE 

RESULTS := $(patsubst %,libwps_%$(EXT),$(TARGETS))

all: $(RESULTS)

libwps_%$(EXT): $(SOURCES)
	@echo CC [$(subst libwps_,,$(subst $(EXT),,$@))]
	@$(CC) $(INCLUDE) $(CFLAGS) -D$(subst libwps_,,$(subst $(EXT),,$@)) $(LDFLAGS) -shared -o $@ $+
	@$(COPY) $@ $(COPY_TO)

clean:
	$(RM) $(RESULTS)
