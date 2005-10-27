#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES= -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I. -I$(OBJDIR)	\
 -I$(BUILDDIR)

DEPFILE = $(OBJDIR)/dep-bootloader
LDS := $(FIRMDIR)/boot.lds

ifdef DEBUG
    DEFINES := -DDEBUG
    CFLAGS += -g
endif

SRC := $(shell cat SOURCES | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P -include "config.h" - )
DIRS = .

ifdef APPEXTRA
   DIRS += $(subst :, ,$(APPEXTRA))
   INCLUDES += $(patsubst %,-I%,$(subst :, ,$(APPEXTRA)))
endif

ifndef VERSION
VERSION=$(shell date +%y%m%d-%H%M)
endif

CFLAGS = $(GCCOPTS) $(INCLUDES) $(TARGET) $(DEFINES)			\
 -DAPPSVERSION=\"$(VERSION)\" $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE}

OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
SOURCES = $(SRC)
LINKFILE = $(OBJDIR)/linkage.lds
MAXINFILE = $(OBJDIR)/romstart.temp
MAXOUTFILE = $(OBJDIR)/romstart

LIBROCKBOX = $(BUILDDIR)/librockbox.a

ifdef DEBUG
all: $(OBJDIR)/bootloader.elf
else
all: $(BUILDDIR)/$(BINARY) $(FLASHFILE)
endif

dep: $(DEPFILE)

$(LINKFILE): $(LDS)
	@echo "Build LDS file"
	@cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - >$@

$(MAXOUTFILE):
	@echo '#include "config.h"' > $(MAXINFILE)
	@echo "ROM_START" >> $(MAXINFILE)
	@cat $(MAXINFILE) | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - > $(MAXOUTFILE)
	@rm $(MAXINFILE)

$(OBJDIR)/bootloader.elf : $(OBJS) $(LINKFILE) $(DEPFILE) $(LIBROCKBOX)
	@echo "LD bootloader.elf"
	$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJS) -L$(BUILDDIR) -L$(BUILDDIR)/firmware -lrockbox -lgcc -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/bootloader.map

$(OBJDIR)/bootloader.bin : $(OBJDIR)/bootloader.elf
	@echo "OBJCOPY $<"
	@$(OC) -O binary $< $@

$(OBJDIR)/bootloader.asm: $(OBJDIR)/bootloader.bin
	$(TOOLSDIR)/sh2d -sh1 $< > $@

$(BUILDDIR)/$(BINARY) : $(OBJDIR)/bootloader.bin
	@echo "Build bootloader file"
	@$(MKFIRMWARE) $< $@

include $(TOOLSDIR)/make.inc

clean:
	@echo "cleaning bootloader"
	@-rm -f $(OBJS) $(BUILDDIR)/$(BINARY) $(OBJDIR)/bootloader.asm \
	$(OBJDIR)/bootloader.bin $(OBJDIR)/bootloader.elf $(OBJDIR)/*.map \
	$(LINKFILE) $(MAXOUTFILE) $(DEPFILE)

-include $(DEPFILE)
