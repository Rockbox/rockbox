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

CFLAGS = -Wall -g -std=gnu99 -Wno-pointer-sign $(DEFINES) -I. $(INCLUDE)
SIM_FLAGS = $(CFLAGS) $(BUILDDATE) -I$(FIRMWARE)/libc/include

OUTPUT = mkrk27boot

# inputs
LIBSOURCES := $(DRIVERS)fat.c $(FIRMWARE)libc/ctype.c  $(FIRMWARE)libc/strtok.c \
    $(FIRMWARE)libc/errno.c $(FIRMWARE)common/strlcpy.c $(FIRMWARE)common/unicode.c \
    $(FIRMWARE)common/file.c $(FIRMWARE)common/dir_uncached.c $(FIRMWARE)common/disk.c ata-sim.c mkrk27boot.c

SOURCES := $(LIBSOURCES) main.c

include ../libtools.make

SIMOBJS = $(addprefix $(OBJDIR),fat.o ctype.o strtok.o errno.o disk.o dir_uncached.o file.o unicode.o strlcpy.o)

$(SIMOBJS):
	@echo CC $<
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CROSS)$(CC) $(SIM_FLAGS) -c -o $@ $<

