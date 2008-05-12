#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

ROOT=../..

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
CFLAGS+=-mno-cygwin
EXT = .exe
else
ifeq ($(findstring MINGW,$(shell uname)),MINGW)
EXT = .exe
CC = mingw32-gcc
endif
endif

COMMON=$(ROOT)/apps/gui/wps_parser.c \
       $(ROOT)/apps/gui/wps_debug.c \
       $(ROOT)/apps/misc.c \
       $(ROOT)/apps/recorder/bmp.c

INCLUDE=-I $(ROOT)/apps/gui \
        -I $(ROOT)/firmware/export \
        -I $(ROOT)/apps \
        -I .

CFLAGS = -g -D__PCTOOL__ -DDEBUG -DROCKBOX_DIR_LEN=9 -DWPS_DIR=\".\" 

all:
	@echo To build, run the buildall.sh script

checkwps: checkwps.c  $(COMMON)
	@echo CC [$(TARGET)]
	@$(CC) $(INCLUDE) $(CFLAGS) $(COMMON) -D$(TARGET) checkwps.c -o $@.$(MODEL)$(EXT)

clean:
	rm -f checkwps.$(MODEL)$(EXT)
