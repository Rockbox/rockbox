#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# $@ is the name to the left of the colon
# $> is the first name to the right of the colon

CC    = sh-elf-gcc 
LD    = sh-elf-ld
AR    = sh-elf-ar
AS    = sh-elf-as
OC    = sh-elf-objcopy

LANGUAGE = english

FIRMWARE := ../firmware
DOCSDIR := ../docs

INCLUDES= -I$(FIRMWARE)/include -I$(FIRMWARE)/export -I. -I$(OBJDIR)

CFLAGS = -O -W -Wall -m1 -nostdlib -ffreestanding -Wstrict-prototypes	\
-fomit-frame-pointer -fschedule-insns $(INCLUDES) $(TARGET) $(DEFINES)	\
-DAPPSVERSION=\"$(VERSION)\" $(EXTRA_DEFINES) -DMEM=${MEM}
AFLAGS += -small -relax

# Check if this is a kind of Recorder
ANYREC = $(findstring RECORDER, $(TARGET))

# Check if this is an Ondio model
ANYONDIO = $(findstring ONDIO, $(TARGET))

# scramble tool
TOOL = scramble

ifndef MEM
    # if MEM is not set, assume 2MB
    MEM=2
endif

ifdef ARCHOS_ROM
    ROMBUILD=-DARCHOS_ROM
endif

LDS := $(FIRMWARE)/app.lds
ROMLDS := $(FIRMWARE)/rom.lds

ifdef DEBUG
    DEFINES := -DDEBUG
    CFLAGS += -g
endif

SRC := $(wildcard *.c)

ifeq ($(TARGET), -DARCHOS_FMRECORDER)
   TOOL_OPT = -fm
endif
ifeq ($(TARGET), -DARCHOS_RECORDERV2)
   TOOL_OPT = -v2
endif
ifeq ($(TARGET), -DARCHOS_ONDIOSP)
   TOOL_OPT = -osp
endif
ifeq ($(TARGET), -DARCHOS_ONDIOFM)
   TOOL_OPT = -ofm
endif


ifeq ($(ANYREC), RECORDER)
   SRC += $(wildcard recorder/*.c)
   CFLAGS += -Irecorder
   OUTNAME = ajbrec.ajz
   # set the ROMTARGET here to make this get built only on recorders
   ROMTARGET := $(OBJDIR)/rombox.ucl
else
ifeq ($(TARGET), -DARCHOS_PLAYER)
   SRC += $(wildcard player/*.c)
   CFLAGS += -Iplayer
   OUTNAME = archos.mod
else
ifeq ($(ANYONDIO), ONDIO)
# use the recorder tree for now
   SRC += $(wildcard recorder/*.c)
   CFLAGS += -Irecorder
   OUTNAME = ajbrec.ajz
else
# some kind of Neo 
   SRC += $(wildcard neo/*.c)
   CFLAGS += -Ineo
   OUTNAME = Rockbox.bin
   TOOL_OPT = -neo
endif
endif
endif

OBJS := $(OBJDIR)/lang.o $(SRC:%.c=$(OBJDIR)/%.o)

LINKFILE = $(OBJDIR)/linkage.lds
LINKROM = $(OBJDIR)/linkrom.lds

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

MAXINFILE = $(OBJDIR)/romstart.temp
MAXOUTFILE = $(OBJDIR)/romstart

ifdef DEBUG
all : $(OBJDIR)/rockbox.elf rocks
else
all : $(OBJDIR)/$(OUTNAME) $(OBJDIR)/rockbox.ucl rocks $(ROMTARGET)
endif

rocks:
	$(MAKE) -C plugins/lib TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR) VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEM}
	$(MAKE) -C plugins TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR) VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEM}

$(OBJDIR)/librockbox.a:
	make -C $(FIRMWARE) TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR)

# MEM should be passed on to this makefile with the chosen memory size given
# in number of MB
$(LINKFILE): $(LDS)
	 cat $< | $(CC) -DMEMORYSIZE=$(MEM) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - >$@

$(LINKROM): $(ROMLDS)
	cat $< | $(CC) -DMEMORYSIZE=$(MEM) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - >$@

$(MAXOUTFILE):
	@echo '#include "config.h"' > $(MAXINFILE)
	@echo "ROM_START" >> $(MAXINFILE)
	@cat $(MAXINFILE) | $(CC) -DMEMORYSIZE=$(MEM) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - > $(MAXOUTFILE)
	@rm $(MAXINFILE)

$(OBJDIR)/rombox.elf : $(OBJS) $(LINKROM) $(OBJDIR)/librockbox.a
	@echo "LD rombox.elf"
	@$(CC) -Os -nostdlib -o $@ $(OBJS) -L$(OBJDIR) -lrockbox -lgcc -L$(FIRMWARE) -T$(LINKROM) -Wl,-Map,$(OBJDIR)/rombox.map

$(OBJDIR)/rockbox.elf : $(OBJS) $(LINKFILE) $(OBJDIR)/librockbox.a
	@echo "LD rockbox.elf"
	@$(CC) -Os -nostdlib -o $@ $(OBJS) -L$(OBJDIR) -lrockbox -lgcc -L$(FIRMWARE) -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/rockbox.map

$(OBJDIR)/rockbox.bin : $(OBJDIR)/rockbox.elf
	@echo "OBJCOPY $<"
	@$(OC) -O binary $< $@

$(OBJDIR)/rombox.bin : $(OBJDIR)/rombox.elf
	@echo "OBJCOPY $<"
	@$(OC) -O binary $< $@

$(OBJDIR)/rockbox.asm: $(OBJDIR)/rockbox.bin
	$(TOOLSDIR)/sh2d -sh1 $< > $@

$(OBJDIR)/$(OUTNAME) : $(OBJDIR)/rockbox.bin
	$(TOOLSDIR)/$(TOOL) $(TOOL_OPT) $< $@

$(OBJDIR)/rockbox.ucl: $(OBJDIR)/rockbox.bin
	@a=`uclpack -h 2>/dev/null`; \
	if test -n "$$a"; then \
	  echo "UCLPACK rockbox" ; \
	  uclpack --best --2e $< $@ >/dev/null 2>&1; \
	else \
	  echo "no uclpack command found, makes a fake rockbox.ucl"; \
	  echo "fake" > $@; \
	fi

$(OBJDIR)/rombox.ucl: $(OBJDIR)/rombox.bin $(MAXOUTFILE)
	@a=`uclpack -h 2>/dev/null`; \
	if test -n "$$a"; then \
	  echo "UCLPACK rombox" ; \
	  uclpack --none $< $@ >/dev/null 2>&1; \
	  if test -s $@; then \
	    perl $(TOOLSDIR)/romsizetest.pl `cat $(MAXOUTFILE)` $<; \
	    if test $$? -ne 0; then \
	      echo "removing UCL file again, making it a fake one"; \
	      echo "fake" > $@; \
	    fi \
	  else \
	    echo "Your uclpack seems to not support --none, making a fake ucl"; \
	    echo "fake" > $@; \
	  fi \
	else \
	  echo "no uclpack command found, makes a fake rombox.ucl"; \
	  echo "fake" > $@; \
	fi

$(OBJDIR)/credits.raw: $(DOCSDIR)/CREDITS
	perl credits.pl < $< > $@

$(OBJDIR)/credits.o: credits.c credits.h $(OBJDIR)/credits.raw
	@mkdir -p `dirname $@`
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.c
	@mkdir -p `dirname $@`
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/build.lang: lang/$(LANGUAGE).lang $(TOOLSDIR)/uplang
	@echo "UPLANG"
	@perl $(TOOLSDIR)/uplang lang/english.lang $< > $@

$(OBJDIR)/lang.o: $(OBJDIR)/build.lang $(TOOLSDIR)/genlang
	@echo "GENLANG"
	@perl -s $(TOOLSDIR)/genlang -p=$(OBJDIR)/lang $<
	@echo "CC lang.c"
	@$(CC) $(CFLAGS) -c $(OBJDIR)/lang.c -o $@

clean:
	@echo "cleaning apps"
	@-rm -f $(OBJS) $(OBJDIR)/$(OUTNAME) $(OBJDIR)/rockbox.asm	\
	$(OBJDIR)/rockbox.bin $(OBJDIR)/rockbox.elf $(OBJDIR)/*.map	\
	$(OBJDIR)/lang.o $(OBJDIR)/build.lang $(OBJDIR)/lang.[ch]	\
	$(OBJDIR)/credits.raw $(LINKFILE) $(OBJDIR)/rockbox.ucl		\
	$(LINKROM) $(OBJDIR)/rombox.ucl $(OBJDIR)/rombox.bin		\
	$(OBJDIR)/rombox.elf $(MAXOUTFILE)
	@-$(RM) -r $(OBJDIR)/$(DEPS)
	@$(MAKE) -C plugins clean

DEPS:=.deps
DEPDIRS:=$(DEPS)
ifeq ($(ANYREC), RECORDER)
    DEPDIRS += $(DEPS)/recorder
else
ifeq ($(TARGET), -DARCHOS_PLAYER)
    DEPDIRS +=  $(DEPS)/player
else
ifeq ($(ANYONDIO), ONDIO)
# use the recorder tree for now
    DEPDIRS += $(DEPS)/recorder
else
# some kind of Neo
    DEPDIRS +=  $(DEPS)/neo
endif
endif
endif

DIRS = $(subst $(DEPS),".",$(DEPDIRS)) 

tags:
	@$(SHELL) -c 'for d in $(DIRS); do { etags -o $(OBJDIR)/TAGS -a $$d/*.[ch]; }; done'

$(OBJDIR)/$(DEPS)/%.d: %.c
	@$(SHELL) -c 'for d in $(DEPDIRS); do { if [ ! -d $(OBJDIR)/$$d ]; then mkdir $(OBJDIR)/$$d; fi; }; done'
	@$(SHELL) -ec '$(CC) -MM $(CFLAGS) $< 2>/dev/null \
		|sed '\''s|\($*\)\.o[ :]*|$(OBJDIR)/\1.o $(<:%.c=%.d) : |g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

ifdef OBJDIR
 -include $(SRC:%.c=$(OBJDIR)/$(DEPS)/%.d)
endif
