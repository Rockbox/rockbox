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

# store output files in this directory:
OBJDIR = .

CFLAGS = -W -Wall -O -m1 -nostdlib -Wstrict-prototypes $(INCLUDES) $(TARGET)

ifdef DEBUG
CFLAGS += -g -DDEBUG
else
CFLAGS += -fomit-frame-pointer -fschedule-insns
endif

AFLAGS += -small -relax

SRC := $(wildcard drivers/*.c common/*.c malloc/*.c *.c)

OBJS := $(SRC:%.c=$(OBJDIR)/%.o) $(OBJDIR)/crt0.o
DEPS:=.deps
DEPDIRS:=$(DEPS) $(DEPS)/drivers $(DEPS)/common $(DEPS)/malloc

OUTPUT = $(OBJDIR)/librockbox.a

$(OUTPUT): $(OBJS)
	$(AR) ruv $@ $+

$(OBJDIR)/%.o: %.c
	@mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-$(RM) -f $(OBJDIR)/$(OBJS) *~ $(OUTPUT)
	-$(RM) -r $(DEPS)

# Special targets
$(OBJDIR)/thread.o: thread.c thread.h
	$(CC) -c -O -fomit-frame-pointer $(CFLAGS) $< -o $@

$(OBJDIR)/crt0.o: crt0.S
	$(AS) $(AFLAGS) -o $@ $<

$(OBJDIR)/$(DEPS)/%.d: %.c
	@$(SHELL) -c 'for d in $(DEPDIRS); do { if [ ! -d $(OBJDIR)/$$d ]; then mkdir $(OBJDIR)/$$d; fi; }; done'
	@echo "Updating dependencies for $<"
	@$(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		|sed '\''s|\($*\)\.o[ :]*|$(OBJDIR)/\1.o $(<:%.c=%.d) : |g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

-include $(SRC:%.c=$(OBJDIR)/$(DEPS)/%.d)
