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

INCLUDES= -I$(FIRMWARE) -I$(FIRMWARE)/common -I$(FIRMWARE)/drivers

# Pick a target to build for
#TARGET = -DARCHOS_PLAYER=1
TARGET = -DARCHOS_PLAYER_OLD=1
#TARGET = -DARCHOS_RECORDER=1

ifdef DEBUG
    DEFINES := -DDEBUG
    LDS := $(FIRMWARE)/gdb.lds
else
    LDS := $(FIRMWARE)/app.lds
endif

CFLAGS = -O -W -Wall -m1 -nostdlib -Wstrict-prototypes -fomit-frame-pointer -fschedule-insns $(INCLUDES) $(TARGET) $(DEFINES)
AFLAGS += -small -relax

SRC := $(wildcard *.c)

ifeq ($(TARGET),-DARCHOS_RECORDER)
   SRC += $(wildcard recorder/*.c)
endif

OBJS := $(SRC:%.c=%.o)

all : archos.mod # archos.asm 

archos.elf : $(OBJS) $(LDS) $(FIRMWARE)/librockbox.a
	$(CC) -Os -nostdlib -o archos.elf $(OBJS) -lrockbox -lgcc -lc -L$(FIRMWARE) -T$(LDS) -Wl,-Map,archos.map

archos.bin : archos.elf
	$(OC) -O binary archos.elf archos.bin

archos.asm: archos.bin
	../tools/sh2d -sh1 archos.bin > archos.asm

archos.mod : archos.bin
	../tools/scramble archos.bin archos.mod

dist:
	tar czvf dist.tar.gz Makefile main.c start.s app.lds

clean:
	-rm -f *.x *.i *.o *.elf *.bin *.map *.mod *.bak *~
	-$(RM) -r $(DEPS)

DEPS:=.deps
DEPDIRS:=$(DEPS) $(DEPS)/drivers $(DEPS)/common

$(DEPS)/%.d: %.c
	@$(SHELL) -c 'for d in $(DEPDIRS); do { if [ ! -d $$d ]; then mkdir $$d; fi; }; done'
	@echo "Updating dependencies for $<"
	@$(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		|sed '\''s|\($*\)\.o[ :]*|\1.o $(<:%.c=%.d) : |g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

-include $(SRC:%.c=$(DEPS)/%.d)
