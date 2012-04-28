#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

CFLAGS += -Wall -W

# Build with "make BOOTOBJS=1" to build with embedded bootloaders and the 
# --install option and interactive mode.  You need the full set of Rockbox 
# bootloaders in this directory - download them from
# http://download.rockbox.org/bootloader/ipod/bootloaders.zip

# Releases of ipodpatcher are created with "make RELEASE=1".  This
# enables BOOTOBJS and uses the VERSION string defined in main.c
ifdef RELEASE
CFLAGS += -DRELEASE
BOOTOBJS=1
endif

ifdef BOOTOBJS
BOOTSRC = ipod1g2g.c ipod3g.c ipod4g.c ipodcolor.c ipodmini1g.c \
	  ipodmini2g.c ipodnano1g.c ipodvideo.c ipodnano2g.c
CFLAGS += -DWITH_BOOTOBJS
endif

ifeq ($(findstring Darwin,$(shell uname)),Darwin)
# additional frameworks to link on on OS X
LDOPTS += -framework CoreFoundation -framework IOKit
endif

LIBSOURCES = ipodpatcher.c fat32format.c arc4.c \
	     ipodio-posix.c ipodio-win32-scsi.c ipodio-win32.c
SOURCES = main.c $(BOOTSRC) $(LIBSOURCES)
ipodpatcher: SOURCES+= ipodio-posix.c
ipodpatcher.exe: SOURCES+= ipodpatcher-rc.o

OUTPUT = ipodpatcher
all: $(OUTPUT)
include ../libtools.make

$(OBJDIR)ipodpatcher-rc.o: ipodpatcher.rc ipodpatcher.manifest
	$(CROSS)$(WINDRES) -i ipodpatcher.rc -o ipodpatcher-rc.o

%.c: bootloader-%.ipod $(BIN2C)
	@echo BIN2C $<
	$(SILENT)$(BIN2C) -i $< $*

%.c: bootloader-%.ipodx $(BIN2C)
	@echo BIN2C $<
	$(SILENT)$(BIN2C) -i $< $*

