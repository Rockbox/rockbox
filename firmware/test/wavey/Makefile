#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

FIRMWARE = ../..
COMMON = $(FIRMWARE)/common
DRIVERS = $(FIRMWARE)/drivers

CC    = gcc
LD    = ld
AR    = ar
AS    = as
OC    = objcopy
scramble = scramble-win32
DEFINES = -DCRT_DISPLAY -DDEBUG
INCLUDES=-I. -I$(FIRMWARE) -I$(COMMON) -I$(DRIVERS)
TARGET_OPTIONS =
CFLAGS = -g -Wall ${TARGET_OPTIONS} -Wstrict-prototypes $(INCLUDES) $(DEFINES)

SRC := playlist.c settings.c panic.c disk.c debug.c harness.c
OBJS := $(SRC:%.c=%.o)

%.o: %.s
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) $(DEFS) -c $<

all : rockbox

rockbox: $(OBJS)
	$(CC) -o $@ ${OBJS}

playlist.o:$(FIRMWARE)/playlist.c
	$(CC) $(CFLAGS) -c $< -o $@

settings.o:$(FIRMWARE)/settings.c
	$(CC) $(CFLAGS) -c $< -o $@

panic.o:$(FIRMWARE)/panic.c
	$(CC) $(CFLAGS) -c $< -o $@

disk.o:$(FIRMWARE)/disk.c
	$(CC) $(CFLAGS) -c $< -o $@

debug.o:$(FIRMWARE)/debug.c
	$(CC) $(CFLAGS) -c $< -o $@

dist:
	tar czvf dist.tar.gz Makefile main.c start.s app.lds

clean:
	-rm -f *.x *.i *.o *.elf *.bin *.map *.mod *.bak *~

