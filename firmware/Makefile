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

# Limits for the built-in sysfont: ASCII for bootloaders, ISO8859-1 for normal builds
ifneq (,$(findstring -DBOOTLOADER,$(EXTRA_DEFINES)))
	MAXCHAR = 127
else
	MAXCHAR = 255
endif

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

SOURCES = $(SRC)
OBJS2 := $(patsubst %.c, $(OBJDIR)/%.o, $(SRC)) $(OBJDIR)/sysfont.o
OBJS = $(patsubst %.S, $(OBJDIR)/%.o, $(OBJS2))
DIRS:=. drivers common
OUTPUT = $(BUILDDIR)/librockbox.a
DEPFILE = $(OBJDIR)/dep-firmware

all: $(OUTPUT) $(EXTRA_TARGETS)

dep: $(DEPFILE)

$(OUTPUT): $(BUILDDIR)/sysfont.h $(OBJS) $(DEPFILE)
	$(call PRINTS,AR+RANLIB $(@F))$(AR) ruv $@ $(OBJS) >/dev/null 2>&1
	$(SILENT)$(RANLIB) $@

include $(TOOLSDIR)/make.inc

clean:
	$(call PRINTS,cleaning firmware)rm -f $(OBJS) $(OUTPUT) $(OBJDIR)/sysfont.c $(DEPFILE)
	$(SILENT)rm -rf $(OBJDIR)/drivers $(OBJDIR)/common

# Special targets
$(OBJDIR)/thread.o: thread.c export/thread.h
	$(call PRINTS,CC thread.c)$(CC) -c -O -fomit-frame-pointer $(CFLAGS) $< -o $@

$(BUILDDIR)/sysfont.h: ../fonts/08-Schumacher-Clean.bdf
	$(call PRINTS,Create sysfont.h)$(TOOLSDIR)/convbdf -l $(MAXCHAR) -h -o $@ $<

$(OBJDIR)/sysfont.o: ../fonts/08-Schumacher-Clean.bdf
	$(call PRINTS,CONVBDF)$(TOOLSDIR)/convbdf -l $(MAXCHAR) -c -o $(OBJDIR)/sysfont.c $<
	$(call PRINTS,CC sysfont.c)$(CC) $(CFLAGS) -c $(OBJDIR)/sysfont.c -o $@

-include $(DEPFILE)
