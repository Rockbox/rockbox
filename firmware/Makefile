#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

HPATH = /usr/local/sh-gcc/bin
CC    = $(HPATH)/sh-elf-gcc 
LD    = $(HPATH)/sh-elf-ld
AR    = $(HPATH)/sh-elf-ar
AS    = $(HPATH)/sh-elf-as
OC    = $(HPATH)/sh-elf-objcopy

INCLUDES=-I.

# Pick a target to build for
TARGET = -DARCHOS_PLAYER=1
#TARGET = -DARCHOS_PLAYER_OLD=1
#TARGET = -DARCHOS_RECORDER=1

CFLAGS = -Os -Wall -m1 -nostdlib -Wstrict-prototypes -fomit-frame-pointer -fschedule-insns $(INCLUDES) $(TARGET)
AFLAGS += -small -relax

SRC := $(wildcard *.c)
OBJS := $(SRC:%.c=%.o)

OBJS := button.o chartables.o lcd.o led.o serial.o system.o ata.o kernel.o

%.o: %.s
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) $(DEFS) -c $<


all : archos.mod # archos.asm 

archos.elf : $(OBJS) app.lds
	$(CC) -nostdlib -o archos.elf $(OBJS) -lgcc -Tapp.lds -Wl,-Map,archos.map

archos.bin : archos.elf
	$(OC) -O binary archos.elf archos.bin

archos.asm: archos.bin
	sh2d -sh1 archos.bin > archos.asm

archos.mod : archos.bin
	scramble archos.bin archos.mod

dist:
	tar czvf dist.tar.gz Makefile main.c start.s app.lds

clean:
	-rm -f *.x *.i *.o *.elf *.bin *.map *.mod *.bak *~
