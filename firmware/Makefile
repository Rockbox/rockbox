#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES=$(TARGET_INC) -Iinclude -I$(FIRMDIR) -Iexport -Icommon -Idrivers -I$(BUILDDIR)

CFLAGS = $(INCLUDES) $(GCCOPTS) $(TARGET) $(BUILDDATE) $(EXTRA_DEFINES) \
 -DMEM=${MEMORYSIZE}

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

SOURCES = $(SRC)
OBJS2 := $(patsubst %.c, $(OBJDIR)/%.o, $(SRC)) $(OBJDIR)/sysfont.o
OBJS = $(patsubst %.S, $(OBJDIR)/%.o, $(OBJS2))
DIRS:=. drivers common
OUTPUT = $(BUILDDIR)/librockbox.a
DEPFILE = $(OBJDIR)/dep-firmware

all: $(OUTPUT) $(EXTRA_TARGETS)

$(DEPFILE): $(BUILDDIR)/sysfont.h

dep: $(DEPFILE)

$(OUTPUT): $(OBJS) $(DEPFILE)
	$(call PRINTS,AR+RANLIB $(@F))$(AR) ruv $@ $(OBJS) >/dev/null 2>&1
	$(SILENT)$(RANLIB) $@

include $(TOOLSDIR)/make.inc

clean:
	$(call PRINTS,cleaning firmware)rm -f $(OBJS) $(OUTPUT) $(OBJDIR)/sysfont.c $(DEPFILE)
	$(SILENT)rm -rf $(OBJDIR)/drivers $(OBJDIR)/common

# Special targets
$(OBJDIR)/thread.o: thread.c export/thread.h
	$(call PRINTS,CC thread.c)$(CC) -c -O -fomit-frame-pointer $(CFLAGS) $< -o $@

$(BUILDDIR)/sysfont.h: ../fonts/rockbox_default.bdf
	$(call PRINTS,Create sysfont.h)$(TOOLSDIR)/convbdf -l 255 -h -o $@ $<

$(OBJDIR)/sysfont.o: ../fonts/rockbox_default.bdf
	$(call PRINTS,CONVBDF)$(TOOLSDIR)/convbdf -l 255 -c -o $(OBJDIR)/sysfont.c $<
	$(call PRINTS,CC sysfont.c)$(CC) $(CFLAGS) -c $(OBJDIR)/sysfont.c -o $@

-include $(DEPFILE)
