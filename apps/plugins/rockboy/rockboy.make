#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

ROCKBOY_SRCDIR = $(APPSDIR)/plugins/rockboy
ROCKBOY_OBJDIR = $(BUILDDIR)/apps/plugins/rockboy

ROCKBOY_SRC := $(call preprocess, $(ROCKBOY_SRCDIR)/SOURCES)
ROCKBOY_OBJ := $(call c2obj, $(ROCKBOY_SRC))

OTHER_SRC += $(ROCKBOY_SRC)

ifeq ($(findstring YES, $(call preprocess, $(APPSDIR)/plugins/BUILD_OVERLAY)), YES)
    ## lowmem targets
    ROCKS += $(ROCKBOY_OBJDIR)/rockboy.ovl
    ROCKBOY_OUTLDS = $(ROCKBOY_OBJDIR)/rockboy.link
    ROCKBOY_OVLFLAGS = -T$(ROCKBOY_OUTLDS) -Wl,--gc-sections -Wl,-Map,$(basename $@).map
else
    ROCKS += $(ROCKBOY_OBJDIR)/rockboy.rock
endif

$(ROCKBOY_OBJDIR)/rockboy.rock: $(ROCKBOY_OBJ)

$(ROCKBOY_OBJDIR)/rockboy.refmap: $(ROCKBOY_OBJ)

$(ROCKBOY_OUTLDS): $(PLUGIN_LDS) $(ROCKBOY_OBJDIR)/rockboy.refmap
	$(call PRINTS,PP $(@F))$(call preprocess2file,$<,$@,-DOVERLAY_OFFSET=$(shell \
		$(TOOLSDIR)/ovl_offset.pl $(ROCKBOY_OBJDIR)/rockboy.refmap))

$(ROCKBOY_OBJDIR)/rockboy.ovl: $(ROCKBOY_OBJ) $(ROCKBOY_OUTLDS)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(ROCKBOY_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(call objcopy,$(basename $@).elf,$@)
