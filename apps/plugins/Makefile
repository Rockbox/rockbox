#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES = -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I$(FIRMDIR)/common \
 -I$(FIRMDIR)/drivers -I$(APPSDIR) -Ilib
CFLAGS = $(GCCOPTS) $(INCLUDES) $(TARGET) $(EXTRA_DEFINES)	\
 -DMEM=${MEMORYSIZE} -DPLUGIN

ifdef APPEXTRA
   INCLUDES += -I$(APPSDIR)/$(APPEXTRA)
endif

ifdef SOFTWARECODECS
   CODECLIBS = -lmad -la52 -lFLAC
endif

LDS := plugin.lds
LINKFILE := $(OBJDIR)/pluginlink.lds
DEPFILE = $(OBJDIR)/dep-plugins

SRC := $(shell cat SOURCES | $(CC) -DMEMORYSIZE=$(MEMORYSIZE)	\
 $(INCLUDES) $(TARGET) $(DEFINES) -E -P -include "config.h" - )
ROCKS := $(SRC:%.c=$(OBJDIR)/%.rock)
SOURCES = $(SRC)
ELFS := $(SRC:%.c=$(OBJDIR)/%.elf)
OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
DIRS = .

all: $(OBJDIR)/libplugin.a $(ROCKS) $(DEPFILE)

$(OBJDIR)/%.elf: $(OBJDIR)/%.o $(LINKFILE) $(OBJDIR)/libplugin.a
	@echo "LD $@"
	@$(CC) $(GCCOPTS) -O -nostdlib -o $@ $< -L$(OBJDIR) $(CODECLIBS) -lplugin -lgcc -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/$*.map

$(OBJDIR)/%.rock : $(OBJDIR)/%.elf
	@echo "OBJCOPY $<"
	@$(OC) -O binary $< $@

include $(TOOLSDIR)/make.inc

$(OBJDIR)/libplugin.a:
	@echo "MAKE in lib"
	@$(MAKE) -C lib TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR) VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEMORYSIZE}

# MEM should be passed on to this makefile with the chosen memory size given
# in number of MB
$(LINKFILE): $(LDS)
	@echo "build $@"
	@cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P - >$@

clean:
	@echo "cleaning plugins"
	@rm -f $(ROCKS) $(LINKFILE) $(OBJDIR)/*.rock $(DEPFILE) $(ELFS) $(OBJS)
	@$(MAKE) -C lib clean

-include $(DEPFILE)
