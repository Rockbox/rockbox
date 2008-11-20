#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 19082 2008-11-10 23:54:24Z zagor $
#

# single-file plugins:
PLUGINS_SRC = $(call preprocess, $(APPSDIR)/plugins/SOURCES)
OTHER_SRC += $(PLUGINS_SRC)
ROCKS := $(PLUGINS_SRC:.c=.rock)
ROCKS := $(subst $(ROOTDIR),$(BUILDDIR),$(ROCKS))

# libplugin.a
PLUGINLIB := $(BUILDDIR)/apps/plugins/libplugin.a
PLUGINLIB_SRC = $(call preprocess, $(APPSDIR)/plugins/lib/SOURCES)
OTHER_SRC += $(PLUGINLIB_SRC)

PLUGINLIB_OBJ := $(PLUGINLIB_SRC:.c=.o)
PLUGINLIB_OBJ := $(PLUGINLIB_OBJ:.S=.o)
PLUGINLIB_OBJ := $(subst $(ROOTDIR),$(BUILDDIR),$(PLUGINLIB_OBJ))

# multifile plugins (subdirs):
PLUGINSUBDIRS := $(call preprocess, $(APPSDIR)/plugins/SUBDIRS)

# include <dir>.make from each subdir (yay!)
$(foreach dir,$(PLUGINSUBDIRS),$(eval include $(dir)/$(notdir $(dir)).make))

### build data / rules
PLUGIN_LDS := $(APPSDIR)/plugins/plugin.lds
PLUGINLINK_LDS := $(BUILDDIR)/apps/plugins/plugin.link

OTHER_INC += -I$(APPSDIR)/plugins

# special compile flags for plugins:
PLUGINFLAGS = -I$(APPSDIR)/plugins -DPLUGIN $(CFLAGS) 

$(ROCKS): $(PLUGINLIB) $(APPSDIR)/plugin.h $(PLUGINLINK_LDS) $(PLUGINBITMAPLIB)

$(PLUGINLIB): $(PLUGINLIB_OBJ)
	$(call PRINTS,AR $(@F))$(AR) rs $@ $^ >/dev/null 2>&1

$(PLUGINLINK_LDS): $(PLUGIN_LDS)
	$(call PRINTS,PP $(@F))
	$(shell mkdir -p $(dir $@))
	$(call preprocess2file,$<,$@,-DLOADADDRESS=$(LOADADDRESS))

$(BUILDDIR)/credits.raw credits.raw: $(DOCSDIR)/CREDITS
	$(call PRINTS,Create credits.raw)perl $(APPSDIR)/plugins/credits.pl < $< > $(BUILDDIR)/$(@F)

# special dependencies
$(BUILDDIR)/apps/plugins/wav2wv.rock: $(BUILDDIR)/apps/codecs/libwavpack.a

# special pattern rule for compiling plugin lib (with -ffunction-sections)
$(BUILDDIR)/apps/plugins/lib/%.o: $(ROOTDIR)/apps/plugins/lib/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PLUGINFLAGS) -ffunction-sections -c $< -o $@

# special pattern rule for compiling plugins with extra flags
$(BUILDDIR)/apps/plugins/%.o: $(ROOTDIR)/apps/plugins/%.c $(PLUGINBITMAPLIB)
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PLUGINFLAGS) -c $< -o $@

ifdef SIMVER
 PLUGINLDFLAGS = $(SHARED_FLAG) # <-- from Makefile
else
 PLUGINLDFLAGS = -T$(PLUGINLINK_LDS) -Wl,--gc-sections -Wl,-Map,$*.map
endif

$(BUILDDIR)/%.rock: $(BUILDDIR)/%.o $(PLUGINLINK_LDS)
	$(call PRINTS,LD $(@F))$(CC) $(PLUGINFLAGS) -o $(BUILDDIR)/$*.elf \
		$(filter %.o, $^) \
		$(filter %.a, $^) \
		-lgcc $(PLUGINLDFLAGS)
ifdef SIMVER
	$(SILENT)cp $(BUILDDIR)/$*.elf $@
else
	$(SILENT)$(OC) -O binary $(BUILDDIR)/$*.elf $@
endif