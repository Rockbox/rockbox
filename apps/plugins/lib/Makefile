#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# ../.. for the plugin.h in the apps dir
# .. for stuff in the plugins dir
# . for stuff in the pluginlib dir
INCLUDES=-I$(APPSDIR) -I.. -I. -I$(FIRMDIR)/include -I$(FIRMDIR)/export	\
 -I$(FIRMDIR)/common -I$(FIRMDIR)/drivers

ifdef APPEXTRA
INCLUDES += -I$(APPSDIR)/$(APPEXTRA)
endif

CFLAGS = $(GCCOPTS) \
$(INCLUDES) $(TARGET) $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE} -DPLUGIN

SRC := $(shell cat SOURCES | gcc -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) $(EXTRA_DEFINES) -E -P -include "config.h" - )
SOURCES = $(SRC)
OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
DEPFILE = $(OBJDIR)/dep-pluginlib
DIRS = .

OUTPUT = $(OBJDIR)/libplugin.a

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	@echo "AR+RANLIB $@"
	@$(AR) ruv $@ $+ >/dev/null 2>&1
	@$(RANLIB) $@

include $(TOOLSDIR)/make.inc

clean:
	@echo "cleaning lib"
	@rm -f $(OBJS) $(OUTPUT) $(DEPFILE)

-include $(DEPFILE)
