#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

GOBAN_SRCDIR := $(APPSDIR)/plugins/goban
GOBAN_BUILDDIR := $(BUILDDIR)/apps/plugins/goban

GOBAN_SRC := $(call preprocess, $(GOBAN_SRCDIR)/SOURCES)
GOBAN_OBJ := $(call c2obj, $(GOBAN_SRC))

OTHER_SRC += $(GOBAN_SRC)

ifndef APP_TYPE
    ### all targets
    ROCKS += $(GOBAN_BUILDDIR)/goban.rock
else
    ### simulator
    ROCKS += $(GOBAN_BUILDDIR)/goban.rock
endif

$(GOBAN_BUILDDIR)/goban.rock: $(GOBAN_OBJ)

$(GOBAN_BUILDDIR)/goban.refmap: $(GOBAN_OBJ)

$(GOBAN_OUTLDS): $(PLUGIN_LDS) $(GOBAN_BUILDDIR)/goban.refmap
	$(call PRINTS,PP $(@F))$(call preprocess2file,$<,$@,-DOVERLAY_OFFSET=$(shell \
		$(TOOLSDIR)/ovl_offset.pl $(GOBAN_BUILDDIR)/goban.refmap))

$(GOBAN_BUILDDIR)/goban.ovl: $(GOBAN_OBJ) $(GOBAN_OUTLDS)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(GOBAN_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(call objcopy,$(basename $@).elf,$@)
