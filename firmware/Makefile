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

INCLUDES=-I. -Icommon -Idrivers

# Pick a target to build for
TARGET = -DARCHOS_PLAYER=1
#TARGET = -DARCHOS_PLAYER_OLD=1
#TARGET = -DARCHOS_RECORDER=1

CFLAGS = -W -Wall -O -m1 -nostdlib -Wstrict-prototypes $(INCLUDES) $(TARGET)

ifdef DEBUG
CFLAGS += -g -DDEBUG
else
CFLAGS += -fomit-frame-pointer -fschedule-insns
endif

AFLAGS += -small -relax

SRC := $(wildcard drivers/*.c common/*.c malloc/*.c *.c)

OBJS := $(SRC:%.c=%.o) crt0.o
DEPS:=.deps
DEPDIRS:=$(DEPS) $(DEPS)/drivers $(DEPS)/common $(DEPS)/malloc

librockbox.a: $(OBJS)
	$(AR) ruv $@ $+

clean:
	-rm -f $(OBJS) *.x *.i *.o *.elf *.bin *.map *.mod *.bak *~
	-$(RM) -r $(DEPS)

# Special targets
thread.o: thread.c thread.h
	$(CC) -c -O -fomit-frame-pointer $(CFLAGS) $<

$(DEPS)/%.d: %.c
	@$(SHELL) -c 'for d in $(DEPDIRS); do { if [ ! -d $$d ]; then mkdir $$d; fi; }; done'
	@echo "Updating dependencies for $<"
	@$(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		|sed '\''s|\($*\)\.o[ :]*|\1.o $(<:%.c=%.d) : |g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

-include $(SRC:%.c=$(DEPS)/%.d)
