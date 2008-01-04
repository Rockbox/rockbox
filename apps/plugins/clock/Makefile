#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $$Id: $$
#

INCLUDES = -I$(APPSDIR) -I.. -I. -I$(APPSDIR)/plugins/lib \
 $(TARGET_INC) -I$(FIRMDIR)/include -I$(FIRMDIR)/export \
 -I$(FIRMDIR)/common -I$(FIRMDIR)/drivers -I$(OUTDIR) -I$(BUILDDIR) \
 -I$(BUILDDIR)/pluginbitmaps
CFLAGS = $(INCLUDES) $(GCCOPTS) $(TARGET) $(EXTRA_DEFINES) \
 -DTARGET_ID=$(TARGET_ID) -DMEM=${MEMORYSIZE} -DPLUGIN

ifdef APPEXTRA
   INCLUDES += $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
endif

LINKFILE := $(OBJDIR)/link.lds
DEPFILE = $(OBJDIR)/dep-clock

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

SOURCES = $(SRC)
OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
DIRS = .

ifndef SIMVER
    LDS := ../plugin.lds
    OUTPUT = $(OUTDIR)/clock.rock
else  ## simulators
    OUTPUT = $(OUTDIR)/clock.rock
endif

all: $(OUTPUT)

ifndef SIMVER
$(OBJDIR)/clock.elf: $(OBJS) $(LINKFILE) $(BITMAPLIBS)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -O -nostdlib -o $@ $(OBJS) -L$(BUILDDIR) -lplugin -lgcc \
           $(LINKBITMAPS) -T$(LINKFILE) -Wl,--gc-sections -Wl,-Map,$(OBJDIR)/clock.map

$(OUTPUT): $(OBJDIR)/clock.elf
	$(call PRINTS,OBJCOPY $(@F))$(OC) -O binary $< $@
else

ifeq ($(SIMVER), x11)
###################################################
# This is the X11 simulator version

$(OUTPUT): $(OBJS)
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) $(SHARED_FLAG) $(OBJS) -L$(BUILDDIR) -lplugin $(LINKBITMAPS) -o $@
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
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) $(SHARED_FLAG) $(OBJS) -L$(BUILDDIR) -lplugin $(LINKBITMAPS) -o $@
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
	$(call PRINTS,DLL $(@F))$(DLLTOOL) $(DLLTOOLFLAGS) -z $(OBJDIR)/$*.def $(OBJS)
	$(SILENT)$(DLLWRAP) $(DLLWRAPFLAGS) --def $(OBJDIR)/$*.def $(OBJS) \
                $(BUILDDIR)/libplugin.a $(BITMAPLIBS) -o $@
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
	$(call PRINTS,build $(@F))cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) \
	$(DEFINES) -E -P - >$@

clean:
	$(call PRINTS,cleaning clock)rm -rf $(OBJDIR)/clock
	$(SILENT)rm -f $(OBJDIR)/clock.* $(DEPFILE)

-include $(DEPFILE)
