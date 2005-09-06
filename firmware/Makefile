#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES=-Iinclude -I$(FIRMDIR) -Iexport -Icommon -Idrivers -I$(BUILDDIR)

CFLAGS = $(GCCOPTS) $(INCLUDES) $(TARGET) $(BUILDDATE) $(EXTRA_DEFINES) \
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

dep: $(DEPFILE)

$(OUTPUT): $(OBJS) $(DEPFILE)
	@echo "AR+RANLIB $@"
	@$(AR) ruv $@ $(OBJS) >/dev/null 2>&1
	@$(RANLIB) $@

include $(TOOLSDIR)/make.inc

clean:
	@echo "cleaning firmware"
	@rm -f $(OBJS) $(OUTPUT) $(OBJDIR)/sysfont.c $(DEPFILE)
	@rm -rf $(OBJDIR)/drivers $(OBJDIR)/common

# Special targets
$(OBJDIR)/thread.o: thread.c export/thread.h
	$(SILENT)echo "CC thread.c"
	$(SILENT)$(CC) -c -O -fomit-frame-pointer $(CFLAGS) $< -o $@

$(OBJDIR)/sysfont.o: ../fonts/rockbox_default.bdf
	@echo "CONVBDF"
	$(SILENT)$(TOOLSDIR)/convbdf -c -o $(OBJDIR)/sysfont.c $<
	@echo "CC sysfont.c"
	$(SILENT)$(CC) $(CFLAGS) -c $(OBJDIR)/sysfont.c -o $@

-include $(DEPFILE)
