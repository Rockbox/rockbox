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

FIRMWARE = ../../../firmware

# Check if this is a kind of Recorder
ANYREC = $(findstring RECORDER, $(TARGET))

# ../.. for the plugin.h in the apps dir
# .. for stuff in the plugins dir
# . for stuff in the pluginlib dir
INCLUDES=-I../.. -I.. -I. -I$(FIRMWARE)/include -I$(FIRMWARE)/export	\
 -I$(FIRMWARE)/common -I$(FIRMWARE)/drivers

ifeq ($(ANYREC), RECORDER)
INCLUDES += -I../../recorder
endif

CFLAGS = -W -Wall -O -m1 -nostdlib -ffreestanding -Wstrict-prototypes	\
$(INCLUDES) $(TARGET) $(EXTRA_DEFINES) -DMEM=${MEM}

ifndef MEM
    # if MEM is not set, assume 2MB
    MEM=2
endif

ifdef DEBUG
CFLAGS += -g -DDEBUG
else
CFLAGS += -fomit-frame-pointer -fschedule-insns
endif

SRC := $(wildcard *.c)

OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
DEPS:=.deps
DEPDIRS:=$(DEPS)

DIRS = $(subst $(DEPS),".",$(DEPDIRS))

OUTPUT = $(OBJDIR)/libplugin.a

ifndef OBJDIR
no_configure:
	@echo "Don't run make here. Run the tools/configure script from your own build"
	@echo "directory, then run make there."
	@echo
	@echo "More help on how to build rockbox can be found here:"
	@echo "http://rockbox.haxx.se/docs/how_to_compile.html"
endif

all: $(OUTPUT)
	@echo "lib done"

$(OUTPUT): $(OBJS)
	@echo "AR $@"
	@$(AR) ruv $@ $+ >/dev/null

$(OBJDIR)/%.o: %.c
	@mkdir -p `dirname $@`
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

tags:
	@$(SHELL) -c 'for d in $(DIRS); do { etags -o $(OBJDIR)/TAGS -a $$d/*.[ch]; }; done'

clean:
	@echo "cleaning lib"
	@rm -f $(OBJS) $(OUTPUT)
	@rm -rf $(OBJDIR)/$(DEPS)

$(OBJDIR)/$(DEPS)/%.d: %.c
	@$(SHELL) -c 'for d in $(DEPDIRS); do { if [ ! -d $(OBJDIR)/$$d ]; then mkdir $(OBJDIR)/$$d; fi; }; done'
	@$(SHELL) -ec '$(CC) -MM $(CFLAGS) $< 2>/dev/null \
		|sed '\''s|\($*\)\.o[ :]*|$(OBJDIR)/\1.o $(<:%.c=%.d) : |g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

ifdef OBJDIR
 -include $(SRC:%.c=$(OBJDIR)/$(DEPS)/%.d)
endif

