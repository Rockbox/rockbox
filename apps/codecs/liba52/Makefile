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
 -I$(FIRMDIR)/common -I$(FIRMDIR)/drivers -I$(APPSDIR)/plugins/liba52/

ifdef APPEXTRA
INCLUDES += -I$(APPSDIR)/$(APPEXTRA)
endif

A52OPTS =  
CFLAGS = $(GCCOPTS) $(A52OPTS) $(INCLUDES) $(TARGET) $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE}

SRC := $(shell cat SOURCES | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P -include "config.h" - )
SOURCES = $(SRC)
OBJS := $(SRC:%.c=$(OBJDIR)/liba52/%.o)
DEPFILE = $(OBJDIR)/dep-liba52
DIRS = 

OUTPUT = $(OBJDIR)/liba52.a

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	@echo "AR $@"
	@$(AR) ruv $@ $+ >/dev/null 2>&1

$(OBJDIR)/liba52/%.o: $(APPSDIR)/codecs/liba52/%.c
	@mkdir -p $(OBJDIR)/liba52
	@echo "CC $<"
	@$(CC) -c -O2 $(CFLAGS) -I$(APPSDIR)/codecs/liba52/ $< -o $@

include $(TOOLSDIR)/make.inc

clean:
	@echo "cleaning liba52"
	@rm -f $(OBJS) $(OUTPUT) $(DEPFILE)

-include $(DEPFILE)
