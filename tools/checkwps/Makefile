#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
CFLAGS+=-mno-cygwin
EXT = .exe
else
ifeq ($(findstring MINGW,$(shell uname)),MINGW)
EXT = .exe
CC = mingw32-gcc
endif
endif

# This is a wrapper for checkwps.make
APPSDIR=.
ROOTDIR=../..
BUILDDIR=.
BINARY=checkwps.$(MODELNAME)$(EXT)
HOSTCC=$(CC)
TARGET=-D$(TARGETNAME)
include checkwps.make

all:
	@echo To build, run the buildall.sh script

checkwps: $(BUILDDIR)/$(BINARY)
	@echo foo > /dev/null

clean:
	rm -f checkwps.$(MODELNAME)$(EXT)
