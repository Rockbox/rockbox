#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

PBMPINCDIR = $(BUILDDIR)/pluginbitmaps

PFLAGS += -I$(PBMPINCDIR)

ifneq ($(strip $(BMP2RB_MONO)),)
PBMP = $(call preprocess, $(APPSDIR)/plugins/bitmaps/mono/SOURCES)
endif
ifneq ($(strip $(BMP2RB_NATIVE)),)
PBMP += $(call preprocess, $(APPSDIR)/plugins/bitmaps/native/SOURCES)
endif
ifneq ($(strip $(BMP2RB_REMOTEMONO)),)
PBMP += $(call preprocess, $(APPSDIR)/plugins/bitmaps/remote_mono/SOURCES)
endif
ifneq ($(strip $(BMP2RB_REMOTENATIVE)),)
PBMP += $(call preprocess, $(APPSDIR)/plugins/bitmaps/remote_native/SOURCES)
endif

ifdef PBMP # does player use bitmaps?

PBMP_BUILD := $(call full_path_subst,$(ROOTDIR)/%,$(BUILDDIR)/%,$(PBMP))

PLUGIN_BITMAPS := $(PBMP_BUILD:%.bmp=%.o)

PLUGINBITMAPLIB := $(BUILDDIR)/apps/plugins/bitmaps/libpluginbitmaps.a
PLUGINBITMAPDIR := $(dir $(PLUGINBITMAPLIB))

PBMPHFILES := $(shell echo $(PBMP_BUILD) | sed  -e 's/\.[0-9x]*\.bmp/.h/g' -e 's/\.bmp/.h/g' | awk "{ gsub(/apps\/plugins\/bitmaps\/(mono|native|remote_mono|remote_native)/, \"pluginbitmaps\"); print }" )

$(PBMPHFILES): $(PLUGIN_BITMAPS)

$(PLUGINBITMAPLIB): $(PLUGIN_BITMAPS)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $+ >/dev/null

# pattern rules to create .c files from .bmp, one for each subdir:
$(BUILDDIR)/apps/plugins/bitmaps/mono/%.c: $(ROOTDIR)/apps/plugins/bitmaps/mono/%.bmp $(TOOLSDIR)/bmp2rb
	$(SILENT)mkdir -p $(dir $@) $(PBMPINCDIR)
	$(call PRINTS,BMP2RB $(<F))$(BMP2RB_MONO) -b -h $(PBMPINCDIR) $< > $@

$(BUILDDIR)/apps/plugins/bitmaps/native/%.c: $(ROOTDIR)/apps/plugins/bitmaps/native/%.bmp $(TOOLSDIR)/bmp2rb
	$(SILENT)mkdir -p $(dir $@) $(PBMPINCDIR)
	$(call PRINTS,BMP2RB $(<F))$(BMP2RB_NATIVE) -b -h $(PBMPINCDIR) $< > $@

$(BUILDDIR)/apps/plugins/bitmaps/remote_mono/%.c: $(ROOTDIR)/apps/plugins/bitmaps/remote_mono/%.bmp $(TOOLSDIR)/bmp2rb
	$(SILENT)mkdir -p $(dir $@) $(PBMPINCDIR)
	$(call PRINTS,BMP2RB $(<F))$(BMP2RB_REMOTEMONO) -b -h $(PBMPINCDIR) $< > $@

$(BUILDDIR)/apps/plugins/bitmaps/remote_native/%.c: $(ROOTDIR)/apps/plugins/bitmaps/remote_native/%.bmp $(TOOLSDIR)/bmp2rb
	$(SILENT)mkdir -p $(dir $@) $(PBMPINCDIR)
	$(call PRINTS,BMP2RB $(<F))$(BMP2RB_REMOTENATIVE) -b -h $(PBMPINCDIR) $< > $@

endif
