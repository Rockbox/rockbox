#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

2600BOX_SRCDIR = $(APPSDIR)/plugins/2600box
2600BOX_OBJDIR = $(BUILDDIR)/apps/plugins/2600box

2600BOX_SRC := $(call preprocess, $(2600BOX_SRCDIR)/SOURCES)
2600BOX_OBJ := $(call c2obj, $(2600BOX_SRC))

OTHER_SRC += $(2600BOX_SRC)

ifeq ($(findstring YES, $(call preprocess, $(APPSDIR)/plugins/BUILD_OVERLAY)), YES)
    ## lowmem targets
    ROCKS += $(2600BOX_OBJDIR)/2600box.ovl
    2600BOX_OUTLDS = $(2600BOX_OBJDIR)/2600box.link
    2600BOX_OVLFLAGS = -T$(2600BOX_OUTLDS) -Wl,--gc-sections -Wl,-Map,$(basename $@).map
else
    ROCKS += $(2600BOX_OBJDIR)/2600box.rock
endif

$(2600BOX_OBJDIR)/2600box.rock: $(2600BOX_OBJ)

$(2600BOX_OBJDIR)/2600box.refmap: $(2600BOX_OBJ)

$(2600BOX_OUTLDS): $(PLUGIN_LDS) $(2600BOX_OBJDIR)/2600box.refmap
	$(call PRINTS,PP $(@F))$(call preprocess2file,$<,$@,-DOVERLAY_OFFSET=$(shell \
		$(TOOLSDIR)/ovl_offset.pl $(2600BOX_OBJDIR)/2600box.refmap))

$(2600BOX_OBJDIR)/2600box.ovl: $(2600BOX_OBJ) $(2600BOX_OUTLDS)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(2600BOX_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(call objcopy,$(basename $@).elf,$@)
