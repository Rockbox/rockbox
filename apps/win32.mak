#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# Win32 GNUSH makefile by Felix Arends
#

#
# USAGE OF THIS MAKEFILE
#
# call this makefile from commandline: make -f win32.mak
#
# to create a recorder target:                 make -f win32.mak RECORDER=1
# to cerate a recorder target with propfonts:  make -f win32.mak RECORDER=1 PROPFONTS=1
# to create a recorder target without games:   make -f win32.mak RECORDER=1 DISABLE_GAMES=1
# to create a player target:                   make -f win32.mak PLAYER=1
# to create an old player target:              make -f win32.mak PLAYER_OLD=1
#

CC    = sh-elf-gcc 
LD    = sh-elf-ld
AR    = sh-elf-ar
AS    = sh-elf-as
OC    = sh-elf-objcopy

LANGUAGE = english

FIRMWARE := ../firmware
TOOLS := ../tools
DOCSDIR := ../docs

INCLUDES= -I$(FIRMWARE)/include -I$(FIRMWARE) -I$(FIRMWARE)/common -I$(FIRMWARE)/drivers -I$(FIRMWARE)/malloc -I.

# Pick a target to build for
TARGET=-DARCHOS_RECORDER=1
ifdef RECORDER
    TARGET=-DARCHOS_RECORDER=1
else
    ifdef PLAYER
        TARGET=-DARCHOS_PLAYER=1
    else
        ifdef PLAYER_OLD
            TARGET=-DARCHOS_PLAYER_OLD=1
        endif
    endif
endif

# store output files in this directory:
OBJDIR = .
DEFINES = -DAPPSVERSION=\"CVS\"

ifdef DISABLE_GAMES
    DEFINES += -DDISABLE_GAMES
endif

ifdef PROPFONTS
    DEFINES  += -DLCD_PROPFONTS
endif

CFLAGS = -O -W -Wall -m1 -nostdlib -ffreestanding -Wstrict-prototypes -fomit-frame-pointer -fschedule-insns $(INCLUDES) $(DEFINES)
AFLAGS += -small -relax

ifdef DEBUG
    DEFINES += -DDEBUG
    CFLAGS += -g
    LDS := $(FIRMWARE)/gdb.lds
else
    LDS := $(FIRMWARE)/app.lds
endif

SRC := $(wildcard *.c)

ifeq ($(TARGET),-DARCHOS_RECORDER=1)
   SRC += $(wildcard recorder/*.c)
   CFLAGS += -Irecorder
   OUTNAME = ajbrec.ajz
else
   SRC += $(wildcard player/*.c)
   CFLAGS += -Iplayer
   OUTNAME = archos.mod
endif

OBJS := $(OBJDIR)/lang.o $(SRC:%.c=$(OBJDIR)/%.o)

all : $(OBJDIR)/$(OUTNAME)

$(OBJDIR)/librockbox.a:
	make -C $(FIRMWARE) -f win32.mak TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR) PLAYER=$(PLAYER) PLAYER_OLD=$(PLAYER_OLD)

$(OBJDIR)/archos.elf : $(OBJS) $(LDS) $(OBJDIR)/librockbox.a
	$(CC) -Os -nostdlib -o $(OBJDIR)/archos.elf $(OBJS) -L$(OBJDIR) -lrockbox -lgcc -L$(FIRMWARE) -T$(LDS) -Wl,-Map,$(OBJDIR)/archos.map

$(OBJDIR)/archos.bin : $(OBJDIR)/archos.elf
	$(OC) -O binary $(OBJDIR)/archos.elf $(OBJDIR)/archos.bin

$(OBJDIR)/archos.asm: $(OBJDIR)/archos.bin
	../tools/sh2d -sh1 $(OBJDIR)/archos.bin > $(OBJDIR)/archos.asm

$(OBJDIR)/$(OUTNAME) : $(OBJDIR)/archos.bin
	$(TOOLS)/scramble $(OBJDIR)/archos.bin $(OBJDIR)/$(OUTNAME)

$(OBJDIR)/credits.raw: $(DOCSDIR)/CREDITS
	perl credits.pl < $< > $(OBJDIR)/$@

$(OBJDIR)/credits.o: $(OBJDIR)/credits.c $(OBJDIR)/credits.h $(OBJDIR)/credits.raw
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(TARGET) -c $< -o $@

$(OBJDIR)/build.lang: lang/$(LANGUAGE).lang
	perl ../tools/uplang lang/english.lang $< > $@

$(OBJDIR)/lang.o: $(OBJDIR)/build.lang
	perl -s ../tools/genlang -p=$(OBJDIR)/lang $<
	$(CC) $(CFLAGS) -c $(OBJDIR)/lang.c -o $@

dist:
	tar czvf dist.tar.gz Makefile main.c start.s app.lds

clean:
	-rm -f $(OBJS) $(OBJDIR)/$(OUTNAME) $(OBJDIR)/archos.asm \
	$(OBJDIR)/archos.bin $(OBJDIR)/archos.elf $(OBJDIR)/archos.map \
	$(OBJDIR)/build.lang $(OBJDIR)/lang.c $(OBJDIR)/lang.h \
	$(OBJDIR)/credits.raw
	make -C $(FIRMWARE) -f win32.mak TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR) clean

DEPS:=.deps
DEPDIRS:=$(DEPS) $(DEPS)/recorder

-include $(SRC:%.c=$(OBJDIR)/$(DEPS)/%.d)
