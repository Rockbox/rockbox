#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES=-I$(APPSDIR) -I.. -I. -I$(FIRMDIR)/include -I$(FIRMDIR)/export	\
 -I$(FIRMDIR)/common -I$(FIRMDIR)/drivers -I$(APPSDIR)/codecs/libFLAC/include

ifdef APPEXTRA
INCLUDES += -I$(APPSDIR)/$(APPEXTRA)
endif

FLACOPTS = -DFLAC__NO_ASM -DFLAC__ALIGN_MALLOC_DATA -DFLAC__INTEGER_ONLY_LIBRARY -DVERSION=\"1.1.2\" -fomit-frame-pointer -funroll-loops -finline-functions -Wall -W -Winline -DFLaC__INLINE=__inline__
CFLAGS = $(GCCOPTS) $(FLACOPTS)\
$(INCLUDES) $(TARGET) $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE}

SRC := $(shell cat SOURCES | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P -include "config.h" - )
SOURCES = $(SRC)
OBJS := $(SRC:%.c=$(OBJDIR)/libFLAC/%.o)
DEPFILE = $(OBJDIR)/dep-libFLAC
DIRS = 

OUTPUT = $(OBJDIR)/libFLAC.a

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	@echo "AR $@"
	@$(AR) ruv $@ $+ >/dev/null 2>&1

$(OBJDIR)/libFLAC/%.o: $(APPSDIR)/codecs/libFLAC/%.c
	@mkdir -p $(OBJDIR)/libFLAC
	@$(CC) -c -O2 $(CFLAGS) -I$(APPSDIR)/codecs/libFLAC/include/FLAC $< -o $@

include $(TOOLSDIR)/make.inc

clean:
	@echo "cleaning libFLAC"
	@rm -f $(OBJS) $(OUTPUT) $(DEPFILE)

-include $(DEPFILE)
