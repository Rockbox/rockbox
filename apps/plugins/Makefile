#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

CC    = sh-elf-gcc
OC    = sh-elf-objcopy

FIRMWARE = ../../firmware

# Check if this is a kind of Recorder
ANYREC = $(findstring RECORDER, $(TARGET))

# Check if this is an Ondio model
ANYONDIO = $(findstring ONDIO, $(TARGET))

INCLUDES = -I$(FIRMWARE)/include -I$(FIRMWARE)/export -I$(FIRMWARE)/common \
-I$(FIRMWARE)/drivers -I.. -Ilib
CFLAGS = -O -W -Wall -m1 -nostdlib -ffreestanding -Wstrict-prototypes	\
$(INCLUDES) $(TARGET) $(EXTRA_DEFINES) -DMEM=${MEM} -DPLUGIN

ifeq ($(ANYREC), RECORDER)
INCLUDES += -I../recorder
endif
ifeq ($(ANYONDIO), ONDIO)
# use the recorder tree for now
INCLUDES += -I../recorder
endif

LDS := plugin.lds
LINKFILE := $(OBJDIR)/pluginlink.lds

SRC := $(wildcard *.c)
ROCKS := $(SRC:%.c=$(OBJDIR)/%.rock)

ifndef OBJDIR
no_configure:
	@echo "Don't run make here. Run the tools/configure script from your own build"
	@echo "directory, then run make there."
	@echo
	@echo "More help on how to build rockbox can be found here:"
	@echo "http://rockbox.haxx.se/docs/how_to_compile.html"
endif

$(OBJDIR)/%.elf: $(OBJDIR)/%.o $(LINKFILE) $(OBJDIR)/libplugin.a
	@echo "LD $@"
	@$(CC) -O -nostdlib -o $@ $< -L$(OBJDIR) -lplugin -lgcc -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/$*.map

$(OBJDIR)/%.rock : $(OBJDIR)/%.elf
	@echo "OBJCOPY $<"
	@$(OC) -O binary $< $@

$(OBJDIR)/%.o: %.c  ../plugin.h Makefile
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

all: $(OBJDIR)/libplugin.a $(ROCKS)
	@echo "plugins done"

$(OBJDIR)/libplugin.a:
	@echo "MAKE in lib"
	@$(MAKE) -C lib TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR) VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEM}

# MEM should be passed on to this makefile with the chosen memory size given
# in number of MB
$(LINKFILE): $(LDS)
	@echo "build $@"
	@$(CC) -DMEMORYSIZE=$(MEM) $(DEFINES) -x c -E -P $< >$@

clean:
	@echo "cleaning plugins"
	@-rm -f $(ROCKS) $(LINKFILE) $(OBJDIR)/*.rock
	@$(MAKE) -C lib clean
