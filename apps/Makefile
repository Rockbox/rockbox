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

INCLUDES= -I$(FIRMWARE)/include -I$(FIRMWARE) -I$(FIRMWARE)/common -I$(FIRMWARE)/drivers -I$(FIRMWARE)/malloc -I. -I$(OBJDIR)

CFLAGS = -O -W -Wall -m1 -nostdlib -ffreestanding -Wstrict-prototypes -fomit-frame-pointer -fschedule-insns $(INCLUDES) $(TARGET) $(DEFINES) -DAPPSVERSION=\"$(VERSION)\" $(EXTRA_DEFINES)
AFLAGS += -small -relax

ifdef DEBUG
    DEFINES := -DDEBUG
    CFLAGS += -g
    LDS := $(FIRMWARE)/gdb.lds
else

# Check if this is a kind of Recorder
ANYREC = $(findstring RECORDER, $(TARGET))

ifeq ($(ANYREC), RECORDER)
    LDS := $(FIRMWARE)/app.lds
else
    LDS := $(FIRMWARE)/player.lds
endif
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

ifndef OBJDIR
no_configure:
	@echo "Don't run make here. Run the tools/configure script from your own build"
	@echo "directory, then run make there."
	@echo
	@echo "More help on how to build rockbox can be found here:"
	@echo "http://rockbox.haxx.se/docs/how_to_compile.html"
endif

all : $(OBJDIR)/$(OUTNAME)

$(OBJDIR)/librockbox.a:
	make -C $(FIRMWARE) TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR)

$(OBJDIR)/archos.elf : $(OBJS) $(LDS) $(OBJDIR)/librockbox.a
	$(CC) -Os -nostdlib -o $(OBJDIR)/archos.elf $(OBJS) -L$(OBJDIR) -lrockbox -lgcc -L$(FIRMWARE) -T$(LDS) -Wl,-Map,$(OBJDIR)/archos.map

$(OBJDIR)/archos.bin : $(OBJDIR)/archos.elf
	$(OC) -O binary $(OBJDIR)/archos.elf $(OBJDIR)/archos.bin

$(OBJDIR)/archos.asm: $(OBJDIR)/archos.bin
	../tools/sh2d -sh1 $(OBJDIR)/archos.bin > $(OBJDIR)/archos.asm

$(OBJDIR)/$(OUTNAME) : $(OBJDIR)/archos.bin
	../tools/scramble $(SCRAMBLE_OPT) $(OBJDIR)/archos.bin $(OBJDIR)/$(OUTNAME)

$(OBJDIR)/credits.raw: $(DOCSDIR)/CREDITS
	perl credits.pl < $< > $@

$(OBJDIR)/credits.o: credits.c credits.h $(OBJDIR)/credits.raw
	@mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.c
	@mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/build.lang: lang/$(LANGUAGE).lang ../tools/uplang
	perl ../tools/uplang lang/english.lang $< > $@

$(OBJDIR)/lang.o: $(OBJDIR)/build.lang ../tools/genlang
	perl -s ../tools/genlang -p=$(OBJDIR)/lang $<
	$(CC) $(CFLAGS) -c $(OBJDIR)/lang.c -o $@

dist:
	tar czvf dist.tar.gz Makefile main.c start.s app.lds

clean:
	-rm -f $(OBJS) $(OBJDIR)/$(OUTNAME) $(OBJDIR)/archos.asm \
	$(OBJDIR)/archos.bin $(OBJDIR)/archos.elf $(OBJDIR)/archos.map \
	$(OBJDIR)/lang.o $(OBJDIR)/build.lang $(OBJDIR)/lang.[ch] \
	$(OBJDIR)/credits.raw
	-$(RM) -r $(OBJDIR)/$(DEPS)

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
