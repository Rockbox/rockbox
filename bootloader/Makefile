#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES= -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I. -I$(OBJDIR)

DEPFILE = $(OBJDIR)/dep-bootloader
LDS := $(FIRMDIR)/boot.lds

ifdef DEBUG
    DEFINES := -DDEBUG
    CFLAGS += -g
endif

SRC := $(shell cat SOURCES | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P -include "config.h" - )
DIRS = .

ifdef APPEXTRA
   DIRS += $(APPEXTRA)
   INCLUDES += -I$(APPEXTRA)
endif

CFLAGS = $(GCCOPTS) $(INCLUDES) $(TARGET) $(DEFINES)			\
 -DAPPSVERSION=\"$(VERSION)\" $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE}

OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
SOURCES = $(SRC)
LINKFILE = $(OBJDIR)/linkage.lds
MAXINFILE = $(OBJDIR)/romstart.temp
MAXOUTFILE = $(OBJDIR)/romstart

ifdef DEBUG
all: $(OBJDIR)/bootloader.elf
else
all: $(OBJDIR)/$(BINARY) $(FLASHFILE)
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

$(OBJDIR)/bootloader.elf : $(OBJS) $(LINKFILE) $(OBJDIR)/librockbox.a $(DEPFILE)
	@echo "LD bootloader.elf"
	$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJS) -L$(OBJDIR) -lrockbox -lgcc -L$(FIRMDIR) -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/bootloader.map

$(OBJDIR)/bootloader.bin : $(OBJDIR)/bootloader.elf
	@echo "OBJCOPY $<"
	@$(OC) -O binary $< $@

$(OBJDIR)/bootloader.asm: $(OBJDIR)/bootloader.bin
	$(TOOLSDIR)/sh2d -sh1 $< > $@

$(OBJDIR)/$(BINARY) : $(OBJDIR)/bootloader.bin
	@echo "Build bootloader file"
	@$(MKFIRMWARE) $< $@

include $(TOOLSDIR)/make.inc

clean:
	@echo "cleaning bootloader"
	@-rm -f $(OBJS) $(OBJDIR)/$(BINARY) $(OBJDIR)/bootloader.asm \
	$(OBJDIR)/bootloader.bin $(OBJDIR)/bootloader.elf $(OBJDIR)/*.map \
	$(LINKFILE) $(MAXOUTFILE) $(DEPFILE)

-include $(DEPFILE)
