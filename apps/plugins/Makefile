#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES = -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I$(FIRMDIR)/common \
-I$(FIRMDIR)/drivers -I$(APPSDIR) -Ilib
CFLAGS = -O -W -Wall -m1 -nostdlib -ffreestanding -Wstrict-prototypes	\
$(INCLUDES) $(TARGET) $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE} -DPLUGIN

ifdef APPEXTRA
   INCLUDES += -I$(APPSDIR)/$(APPEXTRA)
endif

LDS := plugin.lds
LINKFILE := $(OBJDIR)/pluginlink.lds
DEPFILE = $(OBJDIR)/dep-plugins

SRC := $(wildcard *.c)
ROCKS := $(SRC:%.c=$(OBJDIR)/%.rock)
SOURCES = $(SRC)
ELFS := $(SRC:%.c=$(OBJDIR)/%.elf)
OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
DIRS = .

$(OBJDIR)/%.elf: $(OBJDIR)/%.o $(LINKFILE) $(OBJDIR)/libplugin.a $(DEPFILE)
	@echo "LD $@"
	@$(CC) -O -nostdlib -o $@ $< -L$(OBJDIR) -lplugin -lgcc -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/$*.map

$(OBJDIR)/%.rock : $(OBJDIR)/%.elf
	@echo "OBJCOPY $<"
	@$(OC) -O binary $< $@

include $(TOOLSDIR)/make.inc

ifeq ($(ANYONDIO), ONDIO)
all:
	@echo "no plugin support yet"
else
all: $(OBJDIR)/libplugin.a $(ROCKS)
	@echo "plugins done"
endif

$(OBJDIR)/libplugin.a:
	@echo "MAKE in lib"
	@$(MAKE) -C lib TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR) VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEMORYSIZE}

# MEM should be passed on to this makefile with the chosen memory size given
# in number of MB
$(LINKFILE): $(LDS)
	@echo "build $@"
	@$(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(DEFINES) -x c -E -P $< >$@

clean:
	@echo "cleaning plugins"
	@rm -f $(ROCKS) $(LINKFILE) $(OBJDIR)/*.rock $(DEPFILE) $(ELFS) $(OBJS)
	@$(MAKE) -C lib clean
