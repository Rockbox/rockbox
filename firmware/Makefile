#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES=-Iinclude -I$(FIRMDIR) -Iexport -Icommon -Idrivers

CFLAGS = $(GCCOPTS) $(INCLUDES) $(TARGET) $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE}

SRC := $(shell cat SOURCES | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P -include "config.h" - )
SOURCES = $(SRC)
OBJS2 := $(patsubst %.c, $(OBJDIR)/%.o, $(SRC)) $(OBJDIR)/sysfont.o
OBJS = $(patsubst %.S, $(OBJDIR)/%.o, $(OBJS2))
DIRS:=. drivers common
OUTPUT = $(OBJDIR)/librockbox.a
DEPFILE = $(OBJDIR)/dep-firmware

all: $(TOOLSDIR)/convbdf $(OUTPUT) $(EXTRA_TARGETS)

dep: $(DEPFILE)

$(TOOLSDIR)/convbdf:
	$(MAKE) CC=gcc LD=ld -C $(TOOLSDIR)

$(OUTPUT): $(OBJS) $(DEPFILE)
	@echo "AR $@"
	@$(AR) ruv $@ $(OBJS) >/dev/null 2>&1

include $(TOOLSDIR)/make.inc

clean:
	@echo "cleaning firmware"
	@rm -f $(OBJS) $(OUTPUT) $(OBJDIR)/sysfont.c $(DEPFILE)
	@rm -rf $(OBJDIR)/drivers $(OBJDIR)/common

# Special targets
$(OBJDIR)/thread.o: thread.c export/thread.h
	@echo "CC thread.c"
	@$(CC) -c -O -fomit-frame-pointer $(CFLAGS) $< -o $@

$(OBJDIR)/sysfont.o: fonts/clR6x8.bdf
	@echo "CONVBDF"
	@$(TOOLSDIR)/convbdf -c -o $(OBJDIR)/sysfont.c $<
	@echo "CC sysfont.c"
	@$(CC) $(CFLAGS) -c $(OBJDIR)/sysfont.c -o $@

-include $(DEPFILE)
