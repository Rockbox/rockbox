#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES=-I$(APPSDIR) -I.. -I. -I$(FIRMDIR)/include -I$(FIRMDIR)/export	\
 -I$(FIRMDIR)/common -I$(FIRMDIR)/drivers

ifdef APPEXTRA
INCLUDES += -I$(APPSDIR)/$(APPEXTRA)
endif

CFLAGS = $(GCCOPTS) \
$(INCLUDES) $(TARGET) $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE}

SRC := $(shell cat SOURCES | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P -include "config.h" - )
SOURCES = $(SRC)
OBJS := $(SRC:%.c=$(OBJDIR)/libwavpack/%.o)
DEPFILE = $(OBJDIR)/dep-libwavpack
DIRS = 

OUTPUT = $(OBJDIR)/libwavpack.a

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	@echo "AR $@"
	@$(AR) ruv $@ $+ >/dev/null 2>&1

$(OBJDIR)/libwavpack/%.o: $(APPSDIR)/codecs/libwavpack/%.c
	@$(CC) -c -O2 $(CFLAGS) -I$(APPSDIR)/codecs/libwavpack/ $< -o $@

include $(TOOLSDIR)/make.inc

clean:
	@echo "cleaning libwavpack"
	@rm -f $(OBJS) $(OUTPUT) $(DEPFILE)

-include $(DEPFILE)
