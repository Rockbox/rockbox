#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

WOLF3D_SRCDIR = $(APPSDIR)/plugins/wolf3d
WOLF3D_OBJDIR = $(BUILDDIR)/apps/plugins/wolf3d

WOLF3D_SRC := $(call preprocess, $(WOLF3D_SRCDIR)/SOURCES)
WOLF3D_OBJ := $(call c2obj, $(WOLF3D_SRC))

OTHER_SRC += $(WOLF3D_SRC)

ifeq ($(findstring YES, $(call preprocess, $(APPSDIR)/plugins/BUILD_OVERLAY)), YES)
    ## lowmem targets
    ROCKS += $(WOLF3D_OBJDIR)/wolf3d.ovl
    WOLF3D_OUTLDS = $(WOLF3D_OBJDIR)/wolf3d.link
    WOLF3D_OVLFLAGS = -T$(WOLF3D_OUTLDS) -Wl,--gc-sections -Wl,-Map,$(basename $@).map
else
    ROCKS += $(WOLF3D_OBJDIR)/wolf3d.rock
endif

$(WOLF3D_OBJDIR)/wolf3d.rock: $(WOLF3D_OBJ)

$(WOLF3D_OBJDIR)/wolf3d.refmap: $(WOLF3D_OBJ)

$(WOLF3D_OUTLDS): $(PLUGIN_LDS) $(WOLF3D_OBJDIR)/wolf3d.refmap
	$(call PRINTS,PP $(@F))$(call preprocess2file,$<,$@,-DOVERLAY_OFFSET=$(shell \
		$(TOOLSDIR)/ovl_offset.pl $(WOLF3D_OBJDIR)/wolf3d.refmap))

$(WOLF3D_OBJDIR)/wolf3d.ovl: $(WOLF3D_OBJ) $(WOLF3D_OUTLDS)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(WOLF3D_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(call objcopy,$(basename $@).elf,$@)
