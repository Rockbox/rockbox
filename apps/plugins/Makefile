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

INCLUDES = -I$(FIRMWARE)/include -I$(FIRMWARE)/export -I$(FIRMWARE)/common -I$(FIRMWARE)/drivers -I..
CFLAGS = -O -W -Wall -m1 -nostdlib -ffreestanding -Wstrict-prototypes $(INCLUDES) $(TARGET) $(EXTRA_DEFINES) -DMEM=${MEM}

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

$(OBJDIR)/%.elf: $(OBJDIR)/%.o $(LINKFILE)
	$(CC) -O -nostdlib -o $@ $< -lgcc -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/$*.map

$(OBJDIR)/%.rock : $(OBJDIR)/%.elf
	$(OC) -O binary $< $@

$(OBJDIR)/%.o: %.c  ../plugin.h Makefile
	$(CC) $(CFLAGS) -c $< -o $@

all: $(ROCKS)
	@echo done

# MEM should be passed on to this makefile with the chosen memory size given
# in number of MB
$(LINKFILE): $(LDS)
	cat $< | $(CC) -DMEMORYSIZE=$(MEM) $(DEFINES) -E -P - >$@

clean:
	-rm -f $(ROCKS) $(LINKFILE)
