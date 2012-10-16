#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

FIRMWARE = ../../firmware/

DRIVERS = $(FIRMWARE)drivers/
EXPORT = $(FIRMWARE)export/

BUILDDATE=$(shell date -u +'-DYEAR=%Y -DMONTH=%m -DDAY=%d')
INCLUDE = -I$(EXPORT) -I$(FIRMWARE)include -I$(FIRMWARE)target/hosted -I$(FIRMWARE)target/hosted/sdl
DEFINES =  -DTEST_FAT -DDISK_WRITE -DHAVE_FAT16SUPPORT -D__PCTOOL__

SIM_FLAGS = -Wall -g -std=gnu99 -Wno-pointer-sign $(DEFINES) $(BUILDDATE) -I. $(INCLUDE) -I$(FIRMWARE)/libc/include
FLAGS = -Wall -g -std=gnu99 -Wno-pointer-sign $(DEFINES) -I. $(INCLUDE)

OUTPUT = mkrk27boot


# inputs
LIBSOURCES := $(DRIVERS)fat.c $(FIRMWARE)libc/ctype.c  $(FIRMWARE)libc/strtok.c \
    $(FIRWARE)libc/errno.c $(FIRMWARE)common/strlcpy.c $(FIRMWARE)common/unicode.c \
    $(FIRMWARE)common/file.c $(FIRMWARE)common/dir_uncached.c $(FIRMWARE)common/disk.c ata-sim.c mkrk27boot.c

SOURCES := $(LIBSOURCES) main.c

include ../libtools.make

define sim_compile
	@echo CC $<
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CROSS)$(CC) $(SIM_FLAGS) -c -o $@ $<
endef

define compile
	@echo CC $<
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CROSS)$(CC) $(FLAGS) -c -o $@ $<
endef

$(OBJDIR)fat.o: $(DRIVERS)fat.c $(EXPORT)fat.h $(EXPORT)ata.h autoconf.h
	$(sim_compile)

$(OBJDIR)ctype.o: $(FIRMWARE)libc/ctype.c autoconf.h
	$(sim_compile)

$(OBJDIR)strtok.o: $(FIRMWARE)libc/strtok.c $(FIRMWARE)libc/include/string.h autoconf.h
	$(sim_compile)

$(OBJDIR)errno.o: $(FIRMWARE)libc/errno.c $(FIRMWARE)libc/include/errno.h autoconf.h
	$(sim_compile)

$(OBJDIR)disk.o: $(FIRMWARE)common/disk.c autoconf.h
	$(sim_compile)

$(OBJDIR)dir_uncached.o: $(FIRMWARE)common/dir_uncached.c autoconf.h
	$(sim_compile)

$(OBJDIR)file.o: $(FIRMWARE)common/file.c $(FIRMWARE)/include/file.h autoconf.h
	$(sim_compile)

$(OBJDIR)unicode.o: $(FIRMWARE)common/unicode.c autoconf.h
	$(sim_compile)

$(OBJDIR)strlcpy.o: $(FIRMWARE)common/strlcpy.c autoconf.h
	$(sim_compile)

$(OBJDIR)ata-sim.o: ata-sim.c $(EXPORT)ata.h autoconf.h
	$(compile)

$(OBJDIR)mkrk27boot.o: mkrk27boot.c mkrk27boot.h autoconf.h
	$(compile)
