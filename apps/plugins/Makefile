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
CFLAGS = $(GCCOPTS) $(INCLUDES) $(TARGET) $(EXTRA_DEFINES)	\
 -DMEM=${MEMORYSIZE} -DPLUGIN

ifdef APPEXTRA
   INCLUDES += -I$(APPSDIR)/$(APPEXTRA)
endif

ifdef SOFTWARECODECS
   CODECLIBS = -lmad -la52 -lFLAC -lTremor
endif

LDS := plugin.lds
LINKFILE := $(OBJDIR)/pluginlink.lds
DEPFILE = $(OBJDIR)/dep-plugins

SRC := $(shell cat SOURCES | gcc -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) \
 $(TARGET) $(DEFINES) $(EXTRA_DEFINES) -E -P -include "config.h" - )
ROCKS := $(SRC:%.c=$(OBJDIR)/%.rock)
SOURCES = $(SRC)
ELFS := $(SRC:%.c=$(OBJDIR)/%.elf)
OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
# as created by the cross-compiler for win32:
DEFS := $(SRC:%.c=$(OBJDIR)/%.def)
DIRS = .

all: $(OBJDIR)/libplugin.a $(ROCKS) $(DEPFILE)

ifndef SIMVER
$(OBJDIR)/%.elf: $(OBJDIR)/%.o $(LINKFILE) $(OBJDIR)/libplugin.a
	@echo "LD $@"
	@$(CC) $(GCCOPTS) -O -nostdlib -o $@ $< -L$(OBJDIR) $(CODECLIBS) -lplugin -lgcc -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/$*.map

$(OBJDIR)/%.rock : $(OBJDIR)/%.elf
	@echo "OBJCOPY $<"
	@$(OC) -O binary $< $@
else

ifeq ($(SIMVER), x11)
###################################################
# This is the X11 simulator version

$(OBJDIR)/%.rock : $(OBJDIR)/%.o $(APPSDIR)/plugin.h
	@echo "LD $@"
	@$(CC) $(CFLAGS) -shared $< -L$(OBJDIR)  $(CODECLIBS) -lplugin -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	@chmod -x $@
endif

else # end of x11-simulator
###################################################
# This is the win32 simulator version
DLLTOOLFLAGS = --export-all
DLLWRAPFLAGS = -s --entry _DllMain@12 --target=i386-mingw32 -mno-cygwin

$(OBJDIR)/%.rock : $(OBJDIR)/%.o $(APPSDIR)/plugin.h
	@echo "DLL $@"
	@$(DLLTOOL) $(DLLTOOLFLAGS) -z $(OBJDIR)/$*.def $<
	@$(DLLWRAP) $(DLLWRAPFLAGS) --def $(OBJDIR)/$*.def $< $(OBJDIR)/libplugin.a \
                $(patsubst -l%,$(OBJDIR)/lib%.a,$(CODECLIBS)) -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	@chmod -x $@
endif
endif # end of win32-simulator

endif # end of simulator section

include $(TOOLSDIR)/make.inc

$(OBJDIR)/libplugin.a:
	@echo "MAKE in lib"
	@$(MAKE) -C lib TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR) VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEMORYSIZE}

# MEM should be passed on to this makefile with the chosen memory size given
# in number of MB
$(LINKFILE): $(LDS)
	@echo "build $@"
	@cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P - >$@

clean:
	@echo "cleaning plugins"
	@rm -f $(ROCKS) $(LINKFILE) $(OBJDIR)/*.rock $(DEPFILE) $(ELFS) \
	$(OBJS) $(DEFS)
	@$(MAKE) -C lib clean

-include $(DEPFILE)
