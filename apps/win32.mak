#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

CC    = sh-elf-gcc 
LD    = sh-elf-ld
AR    = sh-elf-ar
AS    = sh-elf-as
OC    = sh-elf-objcopy

FIRMWARE := ../firmware

INCLUDES= -I$(FIRMWARE)/include -I$(FIRMWARE) -I$(FIRMWARE)/common -I$(FIRMWARE)/drivers -I$(FIRMWARE)/malloc -I./recorder

# Pick a target to build for
#TARGET = -DARCHOS_PLAYER=1
#TARGET = -DARCHOS_PLAYER_OLD=1
TARGET = -DARCHOS_RECORDER=1

# store output files in this directory:
OBJDIR = .
DEFINES = -DLCD_PROPFONTS

CFLAGS = -O -W -Wall -m1 -nostdlib -Wstrict-prototypes -fomit-frame-pointer -fschedule-insns $(INCLUDES) $(TARGET) $(DEFINES)
AFLAGS += -small -relax

ifdef DEBUG
    DEFINES := -DDEBUG
    CFLAGS += -g
    LDS := $(FIRMWARE)/gdb.lds
else
#ifeq ($(TARGET),-DARCHOS_RECORDER)
    LDS := $(FIRMWARE)/app.lds
#else
#    LDS := $(FIRMWARE)/player.lds
#endif
endif

SRC := $(wildcard *.c)

#ifeq ($(TARGET),-DARCHOS_RECORDER)
   SRC += $(wildcard recorder/*.c)
   CFLAGS += -Irecorder
   OUTNAME = ajbrec.ajz
#else
#   OUTNAME = archos.mod
#endif

OBJS := $(SRC:%.c=$(OBJDIR)/%.o)

all : $(OBJDIR)/$(OUTNAME)

$(OBJDIR)/librockbox.a:
	make -C $(FIRMWARE) -f win32.mak TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR)

$(OBJDIR)/archos.elf : $(OBJS) $(LDS) $(OBJDIR)/librockbox.a
	$(CC) -Os -nostdlib -o $(OBJDIR)/archos.elf $(OBJS) -L$(OBJDIR) -lrockbox -lgcc -L$(FIRMWARE) -T$(LDS) -Wl,-Map,$(OBJDIR)/archos.map

$(OBJDIR)/archos.bin : $(OBJDIR)/archos.elf
	$(OC) -O binary $(OBJDIR)/archos.elf $(OBJDIR)/archos.bin

$(OBJDIR)/archos.asm: $(OBJDIR)/archos.bin
	../tools/sh2d -sh1 $(OBJDIR)/archos.bin > $(OBJDIR)/archos.asm

$(OBJDIR)/$(OUTNAME) : $(OBJDIR)/archos.bin
	scramble $(OBJDIR)/archos.bin $(OBJDIR)/$(OUTNAME)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

dist:
	tar czvf dist.tar.gz Makefile main.c start.s app.lds

clean:
	-rm -f $(OBJS) $(OBJDIR)/$(OUTNAME) $(OBJDIR)/archos.asm \
	$(OBJDIR)/archos.bin $(OBJDIR)/archos.elf $(OBJDIR)/archos.map
	-$(RM) -r $(OBJDIR)/$(DEPS)

DEPS:=.deps
DEPDIRS:=$(DEPS) $(DEPS)/recorder

-include $(SRC:%.c=$(OBJDIR)/$(DEPS)/%.d)
