#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES=-Iinclude -I$(FIRMDIR) -Iexport -Icommon -Idrivers

CFLAGS = -W -Wall -O -m1 -nostdlib -ffreestanding -Wstrict-prototypes	\
$(INCLUDES) $(TARGET) $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE}

ifdef DEBUG
CFLAGS += -g -DDEBUG
else
CFLAGS += -fomit-frame-pointer -fschedule-insns
endif

SRC := $(wildcard drivers/*.c common/*.c *.c)
SRC_S := $(wildcard drivers/*.S common/*.S *.S)
SOURCES = $(SRC) $(SRC_S)
OBJS := $(SRC:%.c=$(OBJDIR)/%.o) $(SRC_S:%.S=$(OBJDIR)/%.o) $(OBJDIR)/sysfont.o
DIRS:=. drivers common
OUTPUT = $(OBJDIR)/librockbox.a
DEPFILE = $(OBJDIR)/dep-firmware

all: $(TOOLSDIR)/convbdf $(OUTPUT) $(EXTRA_TARGETS)

dep: $(DEPFILE)

$(TOOLSDIR)/convbdf:
	$(MAKE) -C $(TOOLSDIR)

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
