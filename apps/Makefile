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

LANGUAGE = english

FIRMWARE := ../firmware
DOCSDIR := ../docs

INCLUDES= -I$(FIRMWARE)/include -I$(FIRMWARE)/export -I. -I$(OBJDIR)

CFLAGS = -O -W -Wall -m1 -nostdlib -ffreestanding -Wstrict-prototypes -fomit-frame-pointer -fschedule-insns $(INCLUDES) $(TARGET) $(DEFINES) -DAPPSVERSION=\"$(VERSION)\" $(EXTRA_DEFINES) -DMEM=${MEM} -DPLUGIN=1
AFLAGS += -small -relax

# Check if this is a kind of Recorder
ANYREC = $(findstring RECORDER, $(TARGET))

ifndef MEM
    # if MEM is not set, assume 2MB
    MEM=2
endif

LDS := $(FIRMWARE)/app.lds

ifdef DEBUG
    DEFINES := -DDEBUG
    CFLAGS += -g
endif

SRC := $(wildcard *.c)

ifeq ($(TARGET), -DARCHOS_FMRECORDER)
   SCRAMBLE_OPT = -fm
endif

ifeq ($(ANYREC), RECORDER)
   SRC += $(wildcard recorder/*.c)
   CFLAGS += -Irecorder
   OUTNAME = ajbrec.ajz
else
   SRC += $(wildcard player/*.c)
   CFLAGS += -Iplayer
   OUTNAME = archos.mod
endif

OBJS := $(OBJDIR)/lang.o $(SRC:%.c=$(OBJDIR)/%.o)

LINKFILE = $(OBJDIR)/linkage.lds

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

all : $(OBJDIR)/$(OUTNAME) rocks

rocks:
	$(MAKE) -C plugins TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR) VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEM}

$(OBJDIR)/librockbox.a:
	make -C $(FIRMWARE) TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR)

# MEM should be passed on to this makefile with the chosen memory size given
# in number of MB
$(LINKFILE): $(LDS)
	cat $< | $(CC) -DMEMORYSIZE=$(MEM) $(DEFINES) -E -P - >$@

$(OBJDIR)/rockbox.elf : $(OBJS) $(LINKFILE) $(OBJDIR)/librockbox.a
	$(CC) -Os -nostdlib -o $(OBJDIR)/rockbox.elf $(OBJS) -L$(OBJDIR) -lrockbox -lgcc -L$(FIRMWARE) -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/rockbox.map

$(OBJDIR)/rockbox.bin : $(OBJDIR)/rockbox.elf
	$(OC) -O binary $(OBJDIR)/rockbox.elf $(OBJDIR)/rockbox.bin

$(OBJDIR)/rockbox.asm: $(OBJDIR)/rockbox.bin
	$(TOOLSDIR)/sh2d -sh1 $(OBJDIR)/rockbox.bin > $(OBJDIR)/rockbox.asm

$(OBJDIR)/$(OUTNAME) : $(OBJDIR)/rockbox.bin
	$(TOOLSDIR)/scramble $(SCRAMBLE_OPT) $(OBJDIR)/rockbox.bin $(OBJDIR)/$(OUTNAME)

$(OBJDIR)/credits.raw: $(DOCSDIR)/CREDITS
	perl credits.pl < $< > $@

$(OBJDIR)/credits.o: credits.c credits.h $(OBJDIR)/credits.raw
	@mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.c
	@mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/build.lang: lang/$(LANGUAGE).lang $(TOOLSDIR)/uplang
	perl $(TOOLSDIR)/uplang lang/english.lang $< > $@

$(OBJDIR)/lang.o: $(OBJDIR)/build.lang $(TOOLSDIR)/genlang
	perl -s $(TOOLSDIR)/genlang -p=$(OBJDIR)/lang $<
	$(CC) $(CFLAGS) -c $(OBJDIR)/lang.c -o $@

clean:
	-rm -f $(OBJS) $(OBJDIR)/$(OUTNAME) $(OBJDIR)/rockbox.asm \
	$(OBJDIR)/rockbox.bin $(OBJDIR)/rockbox.elf $(OBJDIR)/*.map \
	$(OBJDIR)/lang.o $(OBJDIR)/build.lang $(OBJDIR)/lang.[ch] \
	$(OBJDIR)/credits.raw $(LINKFILE) $(OBJDIR)/rockbox.ucl
	-$(RM) -r $(OBJDIR)/$(DEPS)
	$(MAKE) -C plugins clean

DEPS:=.deps
DEPDIRS:=$(DEPS)
ifeq ($(ANYREC), RECORDER)
    DEPDIRS += $(DEPS)/recorder
else
    DEPDIRS +=  $(DEPS)/player
endif

DIRS = $(subst $(DEPS),".",$(DEPDIRS)) 

tags:
	@$(SHELL) -c 'for d in $(DIRS); do { etags -o $(OBJDIR)/TAGS -a $$d/*.[ch]; }; done'

$(OBJDIR)/$(DEPS)/%.d: %.c
	@$(SHELL) -c 'for d in $(DEPDIRS); do { if [ ! -d $(OBJDIR)/$$d ]; then mkdir $(OBJDIR)/$$d; fi; }; done'
	@echo "Updating dependencies for $(OBJDIR)/$<"
	@$(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		|sed '\''s|\($*\)\.o[ :]*|$(OBJDIR)/\1.o $(<:%.c=%.d) : |g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

ifdef OBJDIR
 -include $(SRC:%.c=$(OBJDIR)/$(DEPS)/%.d)
endif
