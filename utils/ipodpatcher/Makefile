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

# additional frameworks to link on on OS X
LDOPTS_OSX = -framework CoreFoundation -framework IOKit

LIBSOURCES = ipodpatcher.c fat32format.c arc4.c \
	     ipodio-posix.c ipodio-win32-scsi.c ipodio-win32.c
SOURCES = main.c $(BOOTSRC)
ipodpatcher: SOURCES+= ipodio-posix.c

OUTPUT = ipodpatcher
include ../libtools.make

ipodpatcher.exe: $(OBJDIR)ipodpatcher-rc.o
$(OBJDIR)ipodpatcher-rc.o: ipodpatcher.rc ipodpatcher.manifest
	@echo WINDRES $(notdir $<)
	$(SILENT)$(CROSS)$(WINDRES) -i $< -o $@

%.c: bootloader-%.ipod $(BIN2C)
	@echo BIN2C $<
	$(SILENT)$(BIN2C) -i $< $*

%.c: bootloader-%.ipodx $(BIN2C)
	@echo BIN2C $<
	$(SILENT)$(BIN2C) -i $< $*

