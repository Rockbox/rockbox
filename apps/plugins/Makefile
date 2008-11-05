#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES = $(TARGET_INC) -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I$(FIRMDIR)/common \
 -I$(FIRMDIR)/drivers -I$(APPSDIR) -Ilib -I$(BUILDDIR)

CFLAGS = $(INCLUDES) $(GCCOPTS) $(TARGET) $(EXTRA_DEFINES)	\
 -DTARGET_ID=$(TARGET_ID) -DMEM=${MEMORYSIZE} -DPLUGIN

ifdef APPEXTRA
   INCLUDES += $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
endif

ifdef SOFTWARECODECS
   CODECLIBS = -lmad -la52 -lffmpegFLAC -ltremor -lwavpack -lmusepack
endif

# Set up the bitmap libraries
BITMAPLIBS =
LINKBITMAPS =
ifneq ($(strip $(BMP2RB_MONO)),)
  BITMAPLIBS += pluginbitmapsmono
  LINKBITMAPS += -lpluginbitmapsmono
endif
ifneq ($(strip $(BMP2RB_NATIVE)),)
  BITMAPLIBS += pluginbitmapsnative
  LINKBITMAPS += -lpluginbitmapsnative
endif
ifneq ($(strip $(BMP2RB_REMOTEMONO)),)
  BITMAPLIBS += pluginbitmapsremotemono
  LINKBITMAPS += -lpluginbitmapsremotemono
endif
ifneq ($(strip $(BMP2RB_REMOTENATIVE)),)
  BITMAPLIBS += pluginbitmapsremotenative
  LINKBITMAPS += -lpluginbitmapsremotenative
endif

LDS := plugin.lds
LINKFILE := $(OBJDIR)/pluginlink.lds
DEPFILE = $(OBJDIR)/dep-plugins

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

# This sets up 'SUBDIRS' based on the directories mentioned in SUBDIRS
include $(TOOLSDIR)/makesubdirs.inc

ROCKS := $(SRC:%.c=$(OBJDIR)/%.rock)
SOURCES = $(SRC)
ELFS := $(SRC:%.c=$(OBJDIR)/%.elf)
OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
# as created by the cross-compiler for win32:
DEFS := $(SRC:%.c=$(OBJDIR)/%.def)
DIRS = .

.PHONY: $(SUBDIRS)
all: build

dep: $(DEPFILE)

build: $(DEPFILE) $(BITMAPLIBS)
	$(call PRINTS,MAKE rocks)$(MAKE) rocks
	$(call PRINTS,MAKE subdirs)$(MAKE) subdirs

rocks: $(ROCKS)

subdirs: $(SUBDIRS)

$(BUILDDIR)/credits.raw: $(DOCSDIR)/CREDITS
	$(call PRINTS,create credits.raw)perl credits.pl < $< > $@

$(OBJDIR)/credits.o: credits.c $(BUILDDIR)/credits.raw
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(<F))$(CC) $(CFLAGS) -I$(OBJDIR) -c $< -o $@

pluginbitmapsmono:
	$(call PRINTS,MAKE in plugins/bitmaps/mono)$(MAKE) -C bitmaps/mono OBJDIR=$(OBJDIR)/bitmaps/mono

pluginbitmapsnative:
	$(call PRINTS,MAKE in plugins/bitmaps/native)$(MAKE) -C bitmaps/native OBJDIR=$(OBJDIR)/bitmaps/native

pluginbitmapsremotemono:
	$(call PRINTS,MAKE in plugins/bitmaps/remote_mono)$(MAKE) -C bitmaps/remote_mono OBJDIR=$(OBJDIR)/bitmaps/remote_mono

pluginbitmapsremotenative:
	$(call PRINTS,MAKE in plugins/bitmaps/remote_native)$(MAKE) -C bitmaps/remote_native OBJDIR=$(OBJDIR)/bitmaps/remote_native

ifndef SIMVER
$(OBJDIR)/%.rock: $(OBJDIR)/%.o $(LINKFILE)
	$(SILENT)$(CC) $(CFLAGS) -o $(OBJDIR)/$*.elf $< -L$(BUILDDIR) \
		$(CODECLIBS) -lplugin $(LINKBITMAPS) -lgcc \
		-T$(LINKFILE) -Wl,--gc-sections -Wl,-Map,$(OBJDIR)/$*.map
	$(call PRINTS,LINK $(@F))$(OC) -O binary $(OBJDIR)/$*.elf $(OBJDIR)/$*.rock

else

ifeq ($(SIMVER), x11)
###################################################
# This is the X11 simulator version

$(OBJDIR)/%.rock : $(OBJDIR)/%.o
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) $(SHARED_FLAG) $< -L$(BUILDDIR)  $(CODECLIBS) -lplugin $(LINKBITMAPS) -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	$(SILENT)chmod -x $@
endif

else # end of x11-simulator
ifeq ($(SIMVER), sdl)
###################################################
# This is the SDL simulator version

$(OBJDIR)/%.rock : $(OBJDIR)/%.o
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) $(SHARED_FLAG) $< -L$(BUILDDIR)  $(CODECLIBS) -lplugin $(LINKBITMAPS) -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	$(SILENT)chmod -x $@
endif

else # end of sdl-simulator
###################################################
# This is the win32 simulator version
DLLTOOLFLAGS = --export-all
DLLWRAPFLAGS = -s --entry _DllMain@12 --target=i386-mingw32 -mno-cygwin

$(OBJDIR)/%.rock : $(OBJDIR)/%.o
	$(call PRINTS,DLL $(@F))$(DLLTOOL) $(DLLTOOLFLAGS) -z $(OBJDIR)/$*.def $<
	$(SILENT)$(DLLWRAP) $(DLLWRAPFLAGS) --def $(OBJDIR)/$*.def $< $(BUILDDIR)/libplugin.a $(BITMAPLIBS) \
                $(patsubst -l%,$(BUILDDIR)/lib%.a,$(CODECLIBS)) -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	$(SILENT)chmod -x $@
endif
endif # end of win32-simulator
endif

endif # end of simulator section

include $(TOOLSDIR)/make.inc

pluginlib:
	$(SILENT)mkdir -p $(OBJDIR)/lib
	$(call PRINTS,MAKE in plugin/lib)$(MAKE) -C lib OBJDIR=$(OBJDIR)/lib

$(LINKFILE): $(LDS)
	$(call PRINTS,build $(@F))cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P - >$@

$(SUBDIRS):
	$(SILENT)mkdir -p $(OBJDIR)/$@
	$(call PRINTS,MAKE in $@)$(MAKE) -C $@ OUTDIR=$(OBJDIR) OBJDIR=$(OBJDIR)/$@ \
        LINKBITMAPS="$(LINKBITMAPS)"

clean:
	$(call PRINTS,cleaning plugins)rm -f $(ROCKS) $(LINKFILE) $(OBJDIR)/*.rock $(DEPFILE) $(ELFS) \
	$(BUILDDIR)/credits.raw $(OBJS) $(DEFS)
	$(SILENT)$(MAKE) -C lib clean OBJDIR=$(OBJDIR)/lib
	$(SILENT)$(MAKE) -C bitmaps/mono clean OBJDIR=$(OBJDIR)/bitmaps/mono
	$(SILENT)$(MAKE) -C bitmaps/native clean OBJDIR=$(OBJDIR)/bitmaps/native
	$(SILENT)$(MAKE) -C bitmaps/remote_mono clean OBJDIR=$(OBJDIR)/bitmaps/remote_mono
	$(SILENT)$(MAKE) -C bitmaps/remote_native clean OBJDIR=$(OBJDIR)/bitmaps/remote_native
	$(SILENT)$(MAKE) -C rockboy clean OBJDIR=$(OBJDIR)/rockboy
	$(SILENT)$(MAKE) -C searchengine clean OBJDIR=$(OBJDIR)/searchengine
	@rm -rf $(BUILDDIR)/pluginbitmaps

-include $(DEPFILE)
