#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES=-I$(APPSDIR) -I.. -I. -I$(FIRMDIR)/include -I$(FIRMDIR)/export	      \
 -I$(FIRMDIR)/common -I$(FIRMDIR)/drivers -I$(APPSDIR)/codecs/libFLAC/include \
 -I$(BUILDDIR)

ifdef APPEXTRA
INCLUDES += -I$(APPSDIR)/$(APPEXTRA)
endif

NEWGCC=$(shell expr $(GCCNUM) ">" 303)

FLACOPTS = -DFLAC__NO_ASM -DFLAC__ALIGN_MALLOC_DATA			\
-DFLAC__INTEGER_ONLY_LIBRARY -DVERSION=\"1.1.2\" -fomit-frame-pointer	\
-funroll-loops -finline-functions -Wall -W -Wno-inline			\
-DFLaC__INLINE=__inline__

ifeq ($(NEWGCC), 1)
  # this is 3.4.X options:
  FLACOPTS += --param large-function-insns=10000
endif

CFLAGS = $(GCCOPTS) $(FLACOPTS)\
$(INCLUDES) $(TARGET) $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE}

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

SOURCES = $(SRC)
OBJS2 := $(SRC:%.c=$(OBJDIR)/%.o)
OBJS = $(patsubst %.S, $(OBJDIR)/%.o, $(OBJS2))
DEPFILE = $(OBJDIR)/dep-libFLAC
DIRS = 

OUTPUT = $(BUILDDIR)/libFLAC.a

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	@echo "AR $@"
	@$(AR) ruv $@ $+ >/dev/null 2>&1

$(OBJDIR)/libFLAC/%.o: $(APPSDIR)/codecs/libFLAC/%.c
	@mkdir -p $(OBJDIR)/libFLAC
	@echo "(libFLAC) CC $<"
	@$(CC) -c -O2 $(CFLAGS) -I$(APPSDIR)/codecs/libFLAC/include/FLAC $< -o $@

include $(TOOLSDIR)/make.inc

clean:
	@echo "cleaning libFLAC"
	@rm -f $(OBJS) $(OUTPUT) $(DEPFILE)

-include $(DEPFILE)
