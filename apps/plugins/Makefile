#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES = -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I$(FIRMDIR)/common \
 -I$(FIRMDIR)/drivers -I$(APPSDIR) -Ilib -I$(BUILDDIR)
CFLAGS = $(GCCOPTS) $(INCLUDES) $(TARGET) $(EXTRA_DEFINES)	\
 -DTARGET_ID=$(TARGET_ID) -DMEM=${MEMORYSIZE} -DPLUGIN

ifdef APPEXTRA
   INCLUDES += $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
endif

ifdef SOFTWARECODECS
   CODECLIBS = -lmad -la52 -lffmpegFLAC -lTremor -lwavpack -lmusepack
endif

# Set up the bitmap libraries
BITMAPLIBS =
LINKBITMAPS =
ifneq ($(strip $(BMP2RB_MONO)),)
  BITMAPLIBS += $(BUILDDIR)/libpluginbitmapsmono.a
  LINKBITMAPS += -lpluginbitmapsmono
endif
ifneq ($(strip $(BMP2RB_NATIVE)),)
  BITMAPLIBS += $(BUILDDIR)/libpluginbitmapsnative.a
  LINKBITMAPS += -lpluginbitmapsnative
endif
ifneq ($(strip $(BMP2RB_REMOTEMONO)),)
  BITMAPLIBS += $(BUILDDIR)/libpluginbitmapsremotemono.a
  LINKBITMAPS += -lpluginbitmapsremotemono
endif
ifneq ($(strip $(BMP2RB_REMOTENATIVE)),)
  BITMAPLIBS += $(BUILDDIR)/libpluginbitmapsremotenative.a
  LINKBITMAPS += -lpluginbitmapsremotenative
endif

LDS := plugin.lds
LINKFILE := $(OBJDIR)/pluginlink.lds
DEPFILE = $(OBJDIR)/dep-plugins

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

ROCKS := $(SRC:%.c=$(OBJDIR)/%.rock)
SOURCES = $(SRC)
ELFS := $(SRC:%.c=$(OBJDIR)/%.elf)
OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
# as created by the cross-compiler for win32:
DEFS := $(SRC:%.c=$(OBJDIR)/%.def)
DIRS = .

#######################################
# Subdirs containing multi-file plugins

#for all targets
SUBDIRS += searchengine databox

#for any recorder, iRiver or iPod model
ifneq (,$(strip $(foreach tgt,RECORDER IRIVER IPOD_COLOR IPOD_VIDEO,$(findstring $(tgt),$(TARGET)))))
ifneq (-DIRIVER_IFP7XX,$(TARGET))
    SUBDIRS += rockboy
endif
endif


.PHONY: $(SUBDIRS)
all: build-bitmaps $(BUILDDIR)/libplugin.a $(ROCKS) $(SUBDIRS) $(DEPFILE)

$(BUILDDIR)/credits.raw: $(DOCSDIR)/CREDITS
	@echo "create credits.raw"
	@perl credits.pl < $< > $@

$(OBJDIR)/credits.o: credits.c $(BUILDDIR)/credits.raw
	@mkdir -p `dirname $@`
	@echo "CC $<"
	@$(CC) $(CFLAGS) -I$(OBJDIR) -c $< -o $@

build-bitmaps:
	@$(MAKE) -C bitmaps OBJDIR=$(OBJDIR)/bitmaps

ifndef SIMVER
$(OBJDIR)/%.elf: $(OBJDIR)/%.o $(LINKFILE) $(BUILDDIR)/libplugin.a $(BITMAPLIBS)
	$(SILENT)(file=`basename $@`; \
	echo "LD $$file"; \
	$(CC) $(GCCOPTS) -O -nostdlib -o $@ $< -L$(BUILDDIR) $(CODECLIBS) -lplugin $(LINKBITMAPS) -lgcc -T$(LINKFILE) -Wl,--gc-sections -Wl,-Map,$(OBJDIR)/$*.map)

$(OBJDIR)/%.rock : $(OBJDIR)/%.elf
	@echo "OBJCOPY "`basename $@`
	@$(OC) -O binary $< $@
else

ifeq ($(SIMVER), x11)
###################################################
# This is the X11 simulator version

$(OBJDIR)/%.rock : $(OBJDIR)/%.o  $(BUILDDIR)/libplugin.a
	@echo "LD "`basename $@`
	@$(CC) $(CFLAGS) -shared $< -L$(BUILDDIR)  $(CODECLIBS) -lplugin $(LINKBITMAPS) -o $@
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

$(OBJDIR)/%.rock : $(OBJDIR)/%.o  $(BUILDDIR)/libplugin.a
	@echo "LD "`basename $@`
	@$(CC) $(CFLAGS) -shared $< -L$(BUILDDIR)  $(CODECLIBS) -lplugin $(LINKBITMAPS) -o $@
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

$(OBJDIR)/%.rock : $(OBJDIR)/%.o  $(BUILDDIR)/libplugin.a
	@echo "DLL "`basename $@`
	@$(DLLTOOL) $(DLLTOOLFLAGS) -z $(OBJDIR)/$*.def $<
	@$(DLLWRAP) $(DLLWRAPFLAGS) --def $(OBJDIR)/$*.def $< $(BUILDDIR)/libplugin.a $(BITMAPLIBS) \
                $(patsubst -l%,$(BUILDDIR)/lib%.a,$(CODECLIBS)) -o $@
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

$(BUILDDIR)/libplugin.a:
	@echo "MAKE in plugin/lib"
	$(SILENT)mkdir -p $(OBJDIR)/lib
	$(SILENT)$(MAKE) -C lib OBJDIR=$(OBJDIR)/lib

$(BUILDDIR)/libpluginbitmapsnative.a:
	@echo "MAKE in plugins/bitmaps/native"
	$(SILENT)mkdir -p $(OBJDIR)/bitmaps/native
	@$(MAKE) -C bitmaps/native OBJDIR=$(OBJDIR)/bitmaps/native

$(BUILDDIR)/libpluginbitmapsmono.a:
	@echo "MAKE in plugins/bitmaps/mono"
	$(SILENT)mkdir -p $(OBJDIR)/bitmaps/mono
	@$(MAKE) -C bitmaps/mono OBJDIR=$(OBJDIR)/bitmaps/mono

$(BUILDDIR)/libpluginbitmapsremotenative.a:
	@echo "MAKE in plugins/bitmaps/remote_native"
	$(SILENT)mkdir -p $(OBJDIR)/bitmaps/remote_native
	@$(MAKE) -C bitmaps/remote_native OBJDIR=$(OBJDIR)/bitmaps/remote_native

$(BUILDDIR)/libpluginbitmapsremotemono.a:
	@echo "MAKE in plugins/bitmaps/remote_mono"
	$(SILENT)mkdir -p $(OBJDIR)/bitmaps/remote_mono
	@$(MAKE) -C bitmaps/remote_mono OBJDIR=$(OBJDIR)/bitmaps/remote_mono

$(LINKFILE): $(LDS)
	@echo "build $@"
	@cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P - >$@

$(SUBDIRS):
	@echo "MAKE in $@"
	$(SILENT)mkdir -p $(OBJDIR)/$@
	$(SILENT)$(MAKE) -C $@ OUTDIR=$(OBJDIR) OBJDIR=$(OBJDIR)/$@

clean:
	@echo "cleaning plugins"
	@rm -f $(ROCKS) $(LINKFILE) $(OBJDIR)/*.rock $(DEPFILE) $(ELFS) \
	$(BUILDDIR)/credits.raw $(OBJS) $(DEFS)
	@$(MAKE) -C lib clean OBJDIR=$(OBJDIR)/lib
	@$(MAKE) -C bitmaps/mono clean OBJDIR=$(OBJDIR)/bitmaps/mono
	@$(MAKE) -C bitmaps/native clean OBJDIR=$(OBJDIR)/bitmaps/native
	@$(MAKE) -C bitmaps/remote_mono clean OBJDIR=$(OBJDIR)/bitmaps/remote_mono
	@$(MAKE) -C bitmaps/remote_native clean OBJDIR=$(OBJDIR)/bitmaps/remote_native
	@$(MAKE) -C rockboy clean OBJDIR=$(OBJDIR)/rockboy
	@$(MAKE) -C searchengine clean OBJDIR=$(OBJDIR)/searchengine

-include $(DEPFILE)
