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
TOOLSDIR = ../tools

INCLUDES=-Iinclude -I. -Icommon -Idrivers -Iexport

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

# use propfonts?
ifdef PROPFONTS
    CFLAGS = -W -Wall -O -m1 -nostdlib -Wstrict-prototypes $(INCLUDES) $(TARGET) -DLCD_PROPFONTS
else
    CFLAGS = -W -Wall -O -m1 -nostdlib -Wstrict-prototypes $(INCLUDES) $(TARGET)
endif

ifdef DEBUG
CFLAGS += -g -DDEBUG
else
CFLAGS += -fomit-frame-pointer -fschedule-insns
endif

SRC := $(wildcard drivers/*.c common/*.c malloc/*.c *.c)

OBJS := $(SRC:%.c=$(OBJDIR)/%.o) $(OBJDIR)/crt0.o $(OBJDIR)/bitswap.o
DEPS:=.deps
DEPDIRS:=$(DEPS) $(DEPS)/drivers $(DEPS)/common $(DEPS)/malloc

ifndef PLAYER
ifndef PLAYER_OLD
  OBJS += $(OBJDIR)/sysfont.o
endif
endif

OUTPUT = $(OBJDIR)/librockbox.a

$(OUTPUT): $(OBJS)
	$(AR) ruv $@ $+

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/sysfont.o: fonts/clR6x8.bdf
	$(TOOLSDIR)/convbdf -c -o $(OBJDIR)/sysfont.c $<
	$(CC) $(CFLAGS) -c $(OBJDIR)/sysfont.c -o $@

clean:
	-rm -f $(OBJS) $(OUTPUT) sysfont.c

# Special targets
$(OBJDIR)/thread.o: thread.c export/thread.h
	$(CC) -c -O -fomit-frame-pointer $(CFLAGS) $< -o $@

-include $(SRC:%.c=$(OBJDIR)/$(DEPS)/%.d)
