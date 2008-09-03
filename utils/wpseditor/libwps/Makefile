#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
ROOT=../../..

OS = w32
CC = gcc
MKDIR = mkdir -p

ifeq ($(findstring MINGW,$(shell uname)),MINGW)
  OS = w32
  CC = mingw32-gcc
  COPY = copy
  RM = rm
endif

ifeq ($(findstring Linux,$(shell uname)),Linux)
  OS = linux
  CC = gcc
  COPY = cp
  RM = rm -f
endif



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

#	   $(ROOT)/apps/recorder/bmp.c
#	   $(ROOT)/apps/abrepeat.c \
#	   $(ROOT)/apps/action.c \
#   	   $(ROOT)/apps/cuesheet.c \
#	   $(ROOT)/apps/gui/statusbar.c \
#	   $(ROOT)/apps/gui/gwps.c \

INCLUDE=-I src/include \
        -I $(ROOT)/apps/gui \
        -I $(ROOT)/firmware/export \
        -I $(ROOT)/firmware/include \
        -I $(ROOT)/apps/recorder \
        -I $(ROOT)/apps \
        -I src

CFLAGS = -g -Wall -D__PCTOOL__ -DWPSEDITOR -DDEBUG -DROCKBOX_DIR_LEN=1 -DBUTTON_REMOTE

all:
	@echo To build, run the buildall.sh script

build: build-$(OS)

build-w32: $(SOURCES)
	@echo CC [$(TARGET)]
	@$(CC) $(INCLUDE) $(CFLAGS) -D$(TARGET) -DTARGET_MODEL=\"$(MODEL)\" -DBUILD_DLL $(SOURCES) -shared -o libwps_$(MODEL).dll

build-linux: $(SOURCES)
	@echo CC [$(TARGET)]
	@$(CC) $(INCLUDE) $(CFLAGS) -D$(TARGET) -DTARGET_MODEL=\"$(MODEL)\" -shared -Wl,-soname,libwps_$(MODEL).so,-olibwps_$(MODEL).so -fPIC $(SOURCES)

clean: clean-$(OS)

clean-w32:
	$(RM) "libwps_$(MODEL).dll"

clean-linux:
	$(RM) "libwps_$(MODEL).so"

shared: shared-$(OS)

shared-w32: $(SOURCES)
	@echo CC [IRIVER_H10_5GB]
	@$(CC) $(INCLUDE) $(CFLAGS) -DIRIVER_H10_5GB -DTARGET_MODEL=\"h10_5gb\" -DBUILD_DLL -shared $(SOURCES) -o ../gui/bin/libwps_h10_5gb.dll

shared-linux: $(SOURCES)
	@echo CC [IRIVER_H10_5GB]
	@$(CC) $(INCLUDE) $(CFLAGS) -DIRIVER_H10_5GB -DTARGET_MODEL=\"h10_5gb\" -shared -Wl,-soname,libwps_h10_5gb.so,-olibwps_h10_5gb.so -fPIC $(SOURCES)
	@$(COPY) libwps_h10_5gb.so ../gui/bin/libwps_h10_5gb.so
