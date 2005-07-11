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

DEPFILE = $(OBJDIR)/dep-bootbox
LDS := $(FIRMDIR)/app.lds

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

LIBROCKBOX = $(BUILDDIR)/librockbox.a

all: $(BUILDDIR)/$(BINARY) $(FLASHFILE)

dep: $(DEPFILE)

$(LINKFILE): $(LDS)
	@echo "Build LDS file"
	@cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - >$@

$(MAXOUTFILE):
	@echo '#include "config.h"' > $(MAXINFILE)
	@echo "ROM_START" >> $(MAXINFILE)
	@cat $(MAXINFILE) | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - > $(MAXOUTFILE)
	@rm $(MAXINFILE)

$(OBJDIR)/bootbox.elf : $(OBJS) $(LINKFILE) $(DEPFILE) $(LIBROCKBOX)
	@echo "LD bootbox.elf"
	$(CC) $(GCCOPTS) -Wl,--gc-sections -Os -nostdlib -o $@ $(OBJS) -L$(BUILDDIR) -L$(BUILDDIR)/firmware -lrockbox -lgcc -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/bootbox.map

$(OBJDIR)/bootbox.bin : $(OBJDIR)/bootbox.elf
	@echo "OBJCOPY $<"
	@$(OC) -O binary $< $@

$(OBJDIR)/bootbox.asm: $(OBJDIR)/bootbox.bin
	$(TOOLSDIR)/sh2d -sh1 $< > $@

$(BUILDDIR)/$(BINARY) : $(OBJDIR)/bootbox.bin
	@echo "Build bootbox file"
	@$(MKFIRMWARE) $< $@

$(FLASHFILE): $(OBJDIR)/bootbox.bin
	@a=`uclpack -h 2>/dev/null`; \
	if test -n "$$a"; then \
	  echo "UCLPACK bootbox" ; \
	  uclpack --best --2e $< $@ >/dev/null 2>&1; \
	else \
	  echo "no uclpack command found, makes a fake bootbox.ucl"; \
	  echo "fake" > $@; \
	fi

include $(TOOLSDIR)/make.inc

clean:
	@echo "cleaning bootbox"
	@-rm -f $(OBJS) $(BUILDDIR)/$(BINARY) $(OBJDIR)/bootbox.asm $(OBJDIR)/rockbox.ucl \
	$(OBJDIR)/bootbox.bin $(OBJDIR)/bootbox.elf $(OBJDIR)/*.map \
	$(LINKFILE) $(MAXOUTFILE) $(DEPFILE) $(FLASHFILE)

-include $(DEPFILE)
