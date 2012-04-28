#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

TARGET_DIR ?= $(shell pwd)/
CFLAGS += -Wall -W -D_LARGEFILE64_SOURCE

OUTPUT = sansapatcher

# inputs
LIBSOURCES := sansapatcher.c sansaio-posix.c sansaio-win32.c
SOURCES := $(LIBSOURCES) main.c
# additional link dependencies for the standalone executable
EXTRADEPS :=

# Releases of sansapatcher are created with "make RELEASE=1".  This
# enables BOOTOBJS and uses the VERSION string defined in main.c
ifdef RELEASE
CFLAGS += -DRELEASE
BOOTOBJS=1
endif

ifdef WITH_BOOTOBJS
BOOTSRC = bootimg_c200.c bootimg_e200.c
SOURCES += $(BOOTSRC)
CFLAGS += -DWITH_BOOTOBJS
endif

include ../libtools.make

# find out if we need to link the manifest resource.
# Since libtools.make sets up BINARY we check it for the file extension .exe.
ifeq ($(findstring exe,$(BINARY)),exe)
$(BINARY): $(OBJDIR)sansapatcher-rc.o
endif

$(OBJDIR)main.o: $(BOOTSRC)
$(OBJDIR)sansapatcher-rc.o: sansapatcher.rc sansapatcher.manifest
	$(WINDRES) -i sansapatcher.rc -o sansapatcher-rc.o

bootimg_c200.c: firmware.mi4 $(BIN2C)
	@echo BIN2C $<
	$(SILENT)$(BIN2C) $< $*

bootimg_e200.c: PP5022.mi4 $(BIN2C)
	@echo BIN2C $< $*
	$(SILENT)$(BIN2C) $< $*

