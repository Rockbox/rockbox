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

INCLUDES=-Iinclude -I. -Icommon -Idrivers

ifndef OBJDIR
	"Don't run make in the firmware dir!"
endif

TOOLSDIR = ../tools

CFLAGS = -W -Wall -O -m1 -nostdlib -ffreestanding -Wstrict-prototypes $(INCLUDES) $(TARGET) $(EXTRA_DEFINES)

ifdef DEBUG
CFLAGS += -g -DDEBUG
else
CFLAGS += -fomit-frame-pointer -fschedule-insns 
endif

SRC := $(wildcard drivers/*.c common/*.c *.c)

OBJS := $(SRC:%.c=$(OBJDIR)/%.o) $(OBJDIR)/crt0.o $(OBJDIR)/bitswap.o
DEPS:=.deps
DEPDIRS:=$(DEPS) $(DEPS)/drivers $(DEPS)/common $(DEPS)/malloc

DIRS = $(subst $(DEPS),".",$(DEPDIRS)) 

OUTPUT = $(OBJDIR)/librockbox.a

ifeq (RECORDER,$(findstring RECORDER, $(CFLAGS)))
	OBJS +=	$(OBJDIR)/sysfont.o
endif

all: $(OUTPUT) $(EXTRA_TARGETS)

$(OUTPUT): $(OBJS)
	$(AR) ruv $@ $+

$(OBJDIR)/%.o: %.c
	@mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.S
	@mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -c $< -o $@

tags:
	@$(SHELL) -c 'for d in $(DIRS); do { etags -o $(OBJDIR)/TAGS -a $$d/*.[ch]; }; done'

clean:
	rm -f $(OBJS) $(OUTPUT) $(OBJDIR)/sysfont.c
	rm -rf $(OBJDIR)/$(DEPS)

# Special targets
$(OBJDIR)/thread.o: thread.c thread.h
	$(CC) -c -O -fomit-frame-pointer $(CFLAGS) $< -o $@

$(OBJDIR)/sysfont.o: fonts/clR6x8.bdf
	$(TOOLSDIR)/convbdf -c -o $(OBJDIR)/sysfont.c $<
	$(CC) $(CFLAGS) -c $(OBJDIR)/sysfont.c -o $@

$(OBJDIR)/$(DEPS)/%.d: %.c
	@$(SHELL) -c 'for d in $(DEPDIRS); do { if [ ! -d $(OBJDIR)/$$d ]; then mkdir $(OBJDIR)/$$d; fi; }; done'
	@echo "Updating dependencies for $<"
	@$(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		|sed '\''s|\($*\)\.o[ :]*|$(OBJDIR)/\1.o $(<:%.c=%.d) : |g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

-include $(SRC:%.c=$(OBJDIR)/$(DEPS)/%.d)
