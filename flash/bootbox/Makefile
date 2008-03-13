#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES= $(TARGET_INC) -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I. -I$(OBJDIR) \
 -I$(BUILDDIR)

DEPFILE = $(OBJDIR)/dep-bootbox
LDS := $(FIRMDIR)/target/$(CPU)/$(MANUFACTURER)/app.lds

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

CFLAGS = $(GCCOPTS) $(INCLUDES) $(TARGET) $(DEFINES)			\
 -DAPPSVERSION=\"$(VERSION)\" $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE}

OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
SOURCES = $(SRC)
LINKFILE = $(OBJDIR)/linkage.lds
MAXINFILE = $(OBJDIR)/romstart.temp
MAXOUTFILE = $(OBJDIR)/romstart

LIBROCKBOX = $(BUILDDIR)/librockbox.a

all: $(BUILDDIR)/$(BINARY) $(FLASHFILE)

dep: $(DEPFILE)

$(LINKFILE): $(LDS)
	$(call PRINTS,Build $(@F))cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - >$@

$(MAXOUTFILE):
	$(SILENT)echo '#include "config.h"' > $(MAXINFILE)
	$(SILENT)echo "ROM_START" >> $(MAXINFILE)
	$(SILENT)cat $(MAXINFILE) | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - > $(MAXOUTFILE)
	$(SILENT)rm $(MAXINFILE)

$(OBJDIR)/bootbox.elf : $(OBJS) $(LINKFILE) $(DEPFILE) $(LIBROCKBOX)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -Wl,--gc-sections -Os -nostdlib -o $@ $(OBJS) -L$(BUILDDIR) -L$(BUILDDIR)/firmware -lrockbox -lgcc -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/bootbox.map

$(OBJDIR)/bootbox.bin : $(OBJDIR)/bootbox.elf
	$(call PRINTS,OBJCOPY $(@F))$(OC) -O binary $< $@

$(OBJDIR)/bootbox.asm: $(OBJDIR)/bootbox.bin
	$(TOOLSDIR)/sh2d -sh1 $< > $@

$(BUILDDIR)/$(BINARY) : $(OBJDIR)/bootbox.bin
	$(call PRINTS,Build bootbox file)$(MKFIRMWARE) $< $@

$(FLASHFILE): $(OBJDIR)/bootbox.bin
	$(call PRINTS,UCLPACK bootbox)$(TOOLSDIR)/uclpack --best --2e $< $@ >/dev/null 2>&1;

include $(TOOLSDIR)/make.inc

clean:
	$(call PRINTS,cleaning bootbox)-rm -f $(OBJS) $(BUILDDIR)/$(BINARY) $(OBJDIR)/bootbox.asm $(OBJDIR)/rockbox.ucl \
	$(OBJDIR)/bootbox.bin $(OBJDIR)/bootbox.elf $(OBJDIR)/*.map \
	$(LINKFILE) $(MAXOUTFILE) $(DEPFILE) $(FLASHFILE)

-include $(DEPFILE)
