#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

PUZZLES_SRCDIR = $(APPSDIR)/plugins/puzzles
PUZZLES_OBJDIR = $(BUILDDIR)/apps/plugins/puzzles

PUZZLES_SRC := $(call preprocess, $(PUZZLES_SRCDIR)/SOURCES)
PUZZLES_OBJ := $(call c2obj, $(PUZZLES_SRC))

OTHER_SRC += $(PUZZLES_SRC)

ifndef APP_TYPE
    ROCKS += $(PUZZLES_OBJDIR)/puzzles.ovl
    PUZZLES_OUTLDS = $(PUZZLES_OBJDIR)/puzzles.link
    PUZZLES_OVLFLAGS = -T$(PUZZLES_OUTLDS) -Wl,--gc-sections -Wl,-Map,$(basename $@).map
else
    ROCKS += $(PUZZLES_OBJDIR)/puzzles.rock
endif

PUZZLESFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -Os -Wno-unused-parameter -Wno-sign-compare

$(PUZZLES_OBJDIR)/puzzles.rock: $(PUZZLES_OBJ) $(TLSFLIB)

$(PUZZLES_OBJDIR)/puzzles.refmap: $(PUZZLES_OBJ) $(TLSFLIB)

$(PUZZLES_OUTLDS): $(PLUGIN_LDS) $(PUZZLES_OBJDIR)/puzzles.refmap
	$(call PRINTS,PP $(@F))$(call preprocess2file,$<,$@,-DOVERLAY_OFFSET=$(shell \
		$(TOOLSDIR)/ovl_offset.pl $(PUZZLES_OBJDIR)/puzzles.refmap))

$(PUZZLES_OBJDIR)/puzzles.ovl: $(PUZZLES_OBJ) $(PUZZLES_OUTLDS) $(TLSFLIB)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(PUZZLES_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(call objcopy,$(basename $@).elf,$@)

# special pattern rule for compiling puzzles with extra flags
$(PUZZLES_OBJDIR)/%.o: $(PUZZLES_SRCDIR)/%.c $(PUZZLES_SRCDIR)/puzzles.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PUZZLESFLAGS) -c $< -o $@
