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
TARGET = -DARCHOS_PLAYER=1
#TARGET = -DARCHOS_PLAYER_OLD=1
#TARGET = -DARCHOS_RECORDER=1

CFLAGS = -Os -W -Wall -m1 -nostdlib -Wstrict-prototypes -fomit-frame-pointer -fschedule-insns $(INCLUDES) $(TARGET) -DDEBUG
AFLAGS += -small -relax

SRC := $(wildcard drivers/*.c common/*.c *.c)
OBJS := $(SRC:%.c=%.o) crt0.o

all : archos.mod # archos.asm 

archos.elf : $(OBJS) $(FIRMWARE)/app.lds
	$(CC) -nostdlib -o archos.elf $(OBJS) -lgcc -lc -T$(FIRMWARE)/app.lds -Wl,-Map,archos.map

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

crt0.o: $(FIRMWARE)/crt0.S

DEPS:=.deps
DEPDIRS:=$(DEPS) $(DEPS)/drivers $(DEPS)/common

$(DEPS)/%.d: %.c
	@$(SHELL) -c 'for d in $(DEPDIRS); do { if [ ! -d $$d ]; then mkdir $$d; fi; }; done'
	@echo "Updating dependencies for $<"
	@$(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		|sed '\''s|\($*\)\.o[ :]*|\1.o $(<:%.c=%.d) : |g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

-include $(SRC:%.c=$(DEPS)/%.d)
