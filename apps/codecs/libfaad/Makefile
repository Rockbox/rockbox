#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES=-I$(APPSDIR) -I.. -I. -I$(FIRMDIR)/include -I$(FIRMDIR)/export	\
 -I$(FIRMDIR)/common -I$(FIRMDIR)/drivers -I$(BUILDDIR)

ifdef APPEXTRA
INCLUDES += -I$(APPSDIR)/$(APPEXTRA)
endif

FAADOPTS = -O2 -Wno-char-subscripts
CFLAGS = $(GCCOPTS) $(FAADOPTS) $(INCLUDES) $(TARGET) $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE}

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

SOURCES = $(SRC)
OBJS2 := $(SRC:%.c=$(OBJDIR)/%.o)
OBJS = $(patsubst %.S, $(OBJDIR)/%.o, $(OBJS2))
DEPFILE = $(OBJDIR)/dep-libfaad
DIRS = 

OUTPUT = $(BUILDDIR)/libfaad.a

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	@echo "AR $@"
	@$(AR) ruv $@ $+ >/dev/null 2>&1

$(OBJDIR)/libfaad/%.o: $(APPSDIR)/codecs/libfaad/%.c
	@echo "(libfaad) CC $<"
	@$(CC) -c $(CFLAGS) -I$(APPSDIR)/codecs/libfaad/ $< -o $@

include $(TOOLSDIR)/make.inc

clean:
	@echo "cleaning libfaad"
	@rm -f $(OBJS) $(OUTPUT) $(DEPFILE)

-include $(DEPFILE)
