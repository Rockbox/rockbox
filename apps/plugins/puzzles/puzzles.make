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

# define this as "yes" to do a monolithic build -- remember to also
# uncomment the sgt-puzzles loader in apps/plugins/SOURCES
PUZZLES_COMBINED =

ifdef PUZZLES_COMBINED
PUZZLES_SRC := $(call preprocess, $(PUZZLES_SRCDIR)/SOURCES, -DCOMBINED)
else
PUZZLES_SRC := $(call preprocess, $(PUZZLES_SRCDIR)/SOURCES)
endif

ifndef PUZZLES_COMBINED
PUZZLES_SHARED_OBJ := $(call c2obj, $(PUZZLES_SRC))
endif

PUZZLES_GAMES_SRC := $(call preprocess, $(PUZZLES_SRCDIR)/SOURCES.games)
PUZZLES_SRC += $(PUZZLES_GAMES_SRC)
PUZZLES_OBJ := $(call c2obj, $(PUZZLES_SRC))
OTHER_SRC += $(PUZZLES_SRC)

ifdef PUZZLES_COMBINED
ifndef APP_TYPE
ROCKS += $(PUZZLES_OBJDIR)/puzzles.ovl
PUZZLES_OUTLDS = $(PUZZLES_OBJDIR)/puzzles.link
PUZZLES_OVLFLAGS = -T$(PUZZLES_OUTLDS) -Wl,--gc-sections -Wl,-Map,$(basename $@).map
else
ROCKS += $(PUZZLES_OBJDIR)/puzzles.rock
endif
else
PUZZLES_ROCKS := $(addprefix $(PUZZLES_OBJDIR)/sgt-, $(notdir $(PUZZLES_GAMES_SRC:.c=.rock)))
ROCKS += $(PUZZLES_ROCKS)
endif

PUZZLESOPTIMIZE := -O2
ifeq ($(MODELNAME), sansac200v2)
PUZZLESOPTIMIZE := -Os # tiny plugin buffer
endif

# we suppress all warnings
PUZZLESFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) $(PUZZLESOPTIMIZE)		\
		-Wno-unused-parameter -Wno-sign-compare -Wno-strict-aliasing -w \
		-DFOR_REAL -I$(PUZZLES_SRCDIR)
ifdef PUZZLES_COMBINED
PUZZLESFLAGS += -DCOMBINED
endif

ifdef PUZZLES_COMBINED
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
else
$(PUZZLES_OBJDIR)/sgt-%.rock: $(PUZZLES_OBJDIR)/%.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
	$(call PRINTS,LD $(@F))$(CC) $(PLUGINFLAGS) -o $(PUZZLES_OBJDIR)/$*.elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(filter-out -Wl%.map, $(PLUGINLDFLAGS)) -Wl,-Map,$(PUZZLES_OBJDIR)/$*.map
	$(SILENT)$(call objcopy,$(PUZZLES_OBJDIR)/$*.elf,$@)
endif

# special pattern rule for compiling puzzles with extra flags
$(PUZZLES_OBJDIR)/%.o: $(PUZZLES_SRCDIR)/%.c $(PUZZLES_SRCDIR)/puzzles.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PUZZLESFLAGS) -c $< -o $@

$(PUZZLES_OBJDIR)/unfinished/%.o: $(PUZZLES_SRCDIR)/unfinished/%.c $(PUZZLES_SRCDIR)/puzzles.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PUZZLESFLAGS) -c $< -o $@
