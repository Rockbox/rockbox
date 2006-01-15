#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES = -I$(APPSDIR) -I.. -I. -I$(FIRMDIR)/include -I$(FIRMDIR)/export \
 -I$(FIRMDIR)/common -I$(FIRMDIR)/drivers -I$(OUTDIR) -I$(BUILDDIR)
CFLAGS = $(GCCOPTS) -O3 $(INCLUDES) $(TARGET) $(EXTRA_DEFINES) \
 -DTARGET_ID=$(TARGET_ID) -DMEM=${MEMORYSIZE} -DPLUGIN

ifdef APPEXTRA
   INCLUDES += $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
endif

LINKFILE := $(OBJDIR)/link.lds
DEPFILE = $(OBJDIR)/dep-searchengine
SRC = searchengine.c parser.c token.c dbinterface.c

SOURCES = $(SRC)
OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
DIRS = .

LDS := ../plugin.lds
OUTPUT = $(OUTDIR)/searchengine.rock

all: $(OUTPUT)

ifndef SIMVER
$(OBJDIR)/searchengine.elf: $(OBJS) $(LINKFILE)
	@echo "LD "`basename $@`
	@$(CC) $(GCCOPTS) -O -nostdlib -o $@ $(OBJS) -L$(BUILDDIR) -lplugin -lgcc \
           -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/searchengine.map

$(OUTPUT): $(OBJDIR)/searchengine.elf
	@echo "OBJCOPY "`basename $@`
	@$(OC) -O binary $< $@
else

ifeq ($(SIMVER), x11)
###################################################
# This is the X11 simulator version

$(OUTPUT): $(OBJS)
	@echo "LD $<"
	@$(CC) $(CFLAGS) -shared $(OBJS) -L$(BUILDDIR) -lplugin -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	@chmod -x $@
endif

else # end of x11-simulator
ifeq ($(SIMVER), sdl)
###################################################
# This is the SDL simulator version

$(OUTPUT): $(OBJS)
	@echo "LD $<"
	@$(CC) $(CFLAGS) -shared $(OBJS) -L$(BUILDDIR) -lplugin -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	@chmod -x $@
endif

else # end of sdl-simulator
###################################################
# This is the win32 simulator version
DLLTOOLFLAGS = --export-all
DLLWRAPFLAGS = -s --entry _DllMain@12 --target=i386-mingw32 -mno-cygwin

$(OUTPUT): $(OBJS)
	@echo "DLL "`basename $@`
	@$(DLLTOOL) $(DLLTOOLFLAGS) -z $(OBJDIR)/$*.def $(OBJS)
	@$(DLLWRAP) $(DLLWRAPFLAGS) --def $(OBJDIR)/$*.def $(OBJS) \
                $(BUILDDIR)/libplugin.a -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	@chmod -x $@
endif
endif # end of win32-simulator
endif
endif # end of simulator section


include $(TOOLSDIR)/make.inc

# MEMORYSIZE should be passed on to this makefile with the chosen memory size
# given in number of MB
$(LINKFILE): $(LDS)
	@echo "build "`basename $@`
	@cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) \
	$(DEFINES) -E -P - >$@

clean:
	@echo "cleaning searchengine"
	@rm -rf $(OBJDIR)/searchengine
	@rm -f $(OBJDIR)/searchengine.* $(DEPFILE)

-include $(DEPFILE)

