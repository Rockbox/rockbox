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

INCLUDES=-Iinclude -I. -Iexport -Icommon -Idrivers

CFLAGS = -W -Wall -O -m1 -nostdlib -ffreestanding -Wstrict-prototypes $(INCLUDES) $(TARGET) $(EXTRA_DEFINES) -DMEM=${MEM}

ifndef MEM
    # if MEM is not set, assume 2MB
    MEM=2
endif

ifdef DEBUG
CFLAGS += -g -DDEBUG
else
CFLAGS += -fomit-frame-pointer -fschedule-insns
endif

SRC := $(wildcard drivers/*.c common/*.c *.c)
SRC_S := $(wildcard drivers/*.S common/*.S *.S)

OBJS := $(SRC:%.c=$(OBJDIR)/%.o) $(SRC_S:%.S=$(OBJDIR)/%.o)
DEPS:=.deps
DEPDIRS:=$(DEPS) $(DEPS)/drivers $(DEPS)/common $(DEPS)/malloc

DIRS = $(subst $(DEPS),".",$(DEPDIRS))

OUTPUT = $(OBJDIR)/librockbox.a

ifeq (RECORDER,$(findstring RECORDER, $(CFLAGS)))
	OBJS +=	$(OBJDIR)/sysfont.o
endif
ifeq (ONDIO,$(findstring ONDIO, $(CFLAGS)))
	OBJS +=	$(OBJDIR)/sysfont.o
endif

ifndef OBJDIR
no_configure:
	@echo "Don't run make here. Run the tools/configure script from your own build"
	@echo "directory, then run make there."
	@echo
	@echo "More help on how to build rockbox can be found here:"
	@echo "http://rockbox.haxx.se/docs/how_to_compile.html"
endif

ifndef TOOLSDIR
	TOOLSDIR=../tools
endif

all: $(TOOLSDIR)/convbdf $(OUTPUT) $(EXTRA_TARGETS)

$(TOOLSDIR)/convbdf:
	$(MAKE) -C $(TOOLSDIR)

$(OUTPUT): $(OBJS)
	@echo "AR $@"
	@$(AR) ruv $@ $+ >/dev/null

$(OBJDIR)/%.o: %.c
	@mkdir -p `dirname $@`
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.S
	@mkdir -p `dirname $@`
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

tags:
	@$(SHELL) -c 'for d in $(DIRS); do { etags -o $(OBJDIR)/TAGS -a $$d/*.[ch]; }; done'

clean:
	@echo "cleaning firmware"
	@rm -f $(OBJS) $(OUTPUT) $(OBJDIR)/sysfont.c
	@rm -rf $(OBJDIR)/$(DEPS)

# Special targets
$(OBJDIR)/thread.o: thread.c export/thread.h
	@echo "CC thread.c"
	@$(CC) -c -O -fomit-frame-pointer $(CFLAGS) $< -o $@

$(OBJDIR)/sysfont.o: fonts/clR6x8.bdf
	@echo "CONVBDF"
	@$(TOOLSDIR)/convbdf -c -o $(OBJDIR)/sysfont.c $<
	@echo "CC sysfont.c"
	@$(CC) $(CFLAGS) -c $(OBJDIR)/sysfont.c -o $@

$(OBJDIR)/$(DEPS)/%.d: %.c
	@$(SHELL) -c 'for d in $(DEPDIRS); do { if [ ! -d $(OBJDIR)/$$d ]; then mkdir $(OBJDIR)/$$d; fi; }; done'
	@$(SHELL) -ec '$(CC) -MM $(CFLAGS) $< 2>/dev/null \
		|sed '\''s|\($*\)\.o[ :]*|$(OBJDIR)/\1.o $(<:%.c=%.d) : |g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

ifdef OBJDIR
 -include $(SRC:%.c=$(OBJDIR)/$(DEPS)/%.d)
endif

