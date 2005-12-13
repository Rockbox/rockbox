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
 -DMEM=${MEMORYSIZE} -DPLUGIN

ifdef APPEXTRA
   INCLUDES += $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
endif

ifdef SOFTWARECODECS
   CODECLIBS = -lmad -la52 -lffmpegFLAC -lTremor -lwavpack -lmusepack
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

#for any recorder and iRiver model
ifneq (,$(strip $(foreach tgt,RECORDER IRIVER,$(findstring $(tgt),$(TARGET)))))
    SUBDIRS += rockboy
endif


.PHONY: $(SUBDIRS)
all: $(BUILDDIR)/libplugin.a $(ROCKS) $(SUBDIRS) $(DEPFILE)

$(OBJDIR)/credits.raw: $(DOCSDIR)/CREDITS
	@echo "create credits.raw"
	@perl credits.pl < $< > $@

$(OBJDIR)/credits.o: credits.c $(OBJDIR)/credits.raw
	@mkdir -p `dirname $@`
	@echo "CC $<"
	@$(CC) $(CFLAGS) -I$(OBJDIR) -c $< -o $@

ifndef SIMVER
$(OBJDIR)/%.elf: $(OBJDIR)/%.o $(LINKFILE) $(BUILDDIR)/libplugin.a
	$(SILENT)(file=`basename $@`; \
	echo "LD $$file"; \
	$(CC) $(GCCOPTS) -O -nostdlib -o $@ $< -L$(BUILDDIR) $(CODECLIBS) -lplugin -lgcc -T$(LINKFILE) -Wl,--gc-sections -Wl,-Map,$(OBJDIR)/$*.map)

$(OBJDIR)/%.rock : $(OBJDIR)/%.elf
	@echo "OBJCOPY "`basename $@`
	@$(OC) -O binary $< $@
else

ifeq ($(SIMVER), x11)
###################################################
# This is the X11 simulator version

$(OBJDIR)/%.rock : $(OBJDIR)/%.o  $(BUILDDIR)/libplugin.a
	@echo "LD "`basename $@`
	@$(CC) $(CFLAGS) -shared $< -L$(BUILDDIR)  $(CODECLIBS) -lplugin -o $@
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

$(OBJDIR)/%.rock : $(OBJDIR)/%.o  $(BUILDDIR)/libplugin.a
	@echo "DLL "`basename $@`
	@$(DLLTOOL) $(DLLTOOLFLAGS) -z $(OBJDIR)/$*.def $<
	@$(DLLWRAP) $(DLLWRAPFLAGS) --def $(OBJDIR)/$*.def $< $(BUILDDIR)/libplugin.a \
                $(patsubst -l%,$(BUILDDIR)/lib%.a,$(CODECLIBS)) -o $@
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

$(BUILDDIR)/libplugin.a:
	@echo "MAKE in plugin/lib"
	$(SILENT)mkdir -p $(OBJDIR)/lib
	$(SILENT)$(MAKE) -C lib OBJDIR=$(OBJDIR)/lib

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
	@$(MAKE) -C rockboy clean OBJDIR=$(OBJDIR)/rockboy
	@$(MAKE) -C searchengine clean OBJDIR=$(OBJDIR)/searchengine

-include $(DEPFILE)
