#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

IMGVSRCDIR := $(APPSDIR)/plugins/imageviewer
IMGVBUILDDIR := $(BUILDDIR)/apps/plugins/imageviewer

ROCKS += $(IMGVBUILDDIR)/imageviewer.rock

IMGV_SRC := $(call preprocess, $(IMGVSRCDIR)/SOURCES)
IMGV_OBJ := $(call c2obj, $(IMGV_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(IMGV_SRC)

$(IMGVBUILDDIR)/imageviewer.rock: $(IMGV_OBJ)

IMGDECFLAGS = $(PLUGINFLAGS) -DIMGDEC

# include decoder's make from each subdir
IMGVSUBDIRS := $(call preprocess, $(IMGVSRCDIR)/SUBDIRS)
$(foreach dir,$(IMGVSUBDIRS),$(eval include $(dir)/$(notdir $(dir)).make))

IMGDECLDFLAGS = -T$(PLUGINLINK_LDS) -Wl,--gc-sections -Wl,-Map,$(IMGVBUILDDIR)/$*.refmap

ifndef APP_TYPE
    IMGDEC_OVLFLAGS = $(PLUGINLDFLAGS) -Wl,-n -Wl,-r -Wl,-Map,$(IMGVBUILDDIR)/$*.map
else
    IMGDEC_OVLFLAGS = $(PLUGINLDFLAGS) -Wl,-Map,$(IMGVBUILDDIR)/$*.map
endif

$(IMGVBUILDDIR)/%.ovl:
	$(call PRINTS,LD $(@F))$(CC) $(IMGDEC_OVLFLAGS) -o $@ \
		$(filter-out $(PLUGIN_CRT0),$(filter %.o, $^)) \
		$(filter %.a, $+) \
		-lgcc $(IMGDECFLAGS)
	$(call PRINTS,STRIP $(subst $(ROOTDIR)/,,$@))$(STRIP) \
		--strip-unneeded --strip-debug \
		-R .comment -R .ARM.attributes $@
