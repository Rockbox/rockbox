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

# Hack to suppress all warnings:
PUZZLESFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -Os		\
		-Wno-unused-parameter -Wno-sign-compare -Wno-strict-aliasing -w	\
		-DFOR_REAL
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
# todo: figure out how to do this with wildcards
$(PUZZLES_OBJDIR)/sgt-blackbox.rock: $(PUZZLES_OBJDIR)/blackbox.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-bridges.rock: $(PUZZLES_OBJDIR)/bridges.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-cube.rock: $(PUZZLES_OBJDIR)/cube.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-dominosa.rock: $(PUZZLES_OBJDIR)/dominosa.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-fifteen.rock: $(PUZZLES_OBJDIR)/fifteen.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-filling.rock: $(PUZZLES_OBJDIR)/filling.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-flip.rock: $(PUZZLES_OBJDIR)/flip.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-flood.rock: $(PUZZLES_OBJDIR)/flood.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-galaxies.rock: $(PUZZLES_OBJDIR)/galaxies.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-guess.rock: $(PUZZLES_OBJDIR)/guess.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-inertia.rock: $(PUZZLES_OBJDIR)/inertia.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-keen.rock: $(PUZZLES_OBJDIR)/keen.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-lightup.rock: $(PUZZLES_OBJDIR)/lightup.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-loopy.rock: $(PUZZLES_OBJDIR)/loopy.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-magnets.rock: $(PUZZLES_OBJDIR)/magnets.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-map.rock: $(PUZZLES_OBJDIR)/map.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-mines.rock: $(PUZZLES_OBJDIR)/mines.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-net.rock: $(PUZZLES_OBJDIR)/net.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-netslide.rock: $(PUZZLES_OBJDIR)/netslide.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-palisade.rock: $(PUZZLES_OBJDIR)/palisade.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-pattern.rock: $(PUZZLES_OBJDIR)/pattern.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-pearl.rock: $(PUZZLES_OBJDIR)/pearl.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-pegs.rock: $(PUZZLES_OBJDIR)/pegs.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-range.rock: $(PUZZLES_OBJDIR)/range.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-rect.rock: $(PUZZLES_OBJDIR)/rect.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-samegame.rock: $(PUZZLES_OBJDIR)/samegame.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-signpost.rock: $(PUZZLES_OBJDIR)/signpost.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-singles.rock: $(PUZZLES_OBJDIR)/singles.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-sixteen.rock: $(PUZZLES_OBJDIR)/sixteen.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-slant.rock: $(PUZZLES_OBJDIR)/slant.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-solo.rock: $(PUZZLES_OBJDIR)/solo.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-tents.rock: $(PUZZLES_OBJDIR)/tents.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-towers.rock: $(PUZZLES_OBJDIR)/towers.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-tracks.rock: $(PUZZLES_OBJDIR)/tracks.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-twiddle.rock: $(PUZZLES_OBJDIR)/twiddle.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-undead.rock: $(PUZZLES_OBJDIR)/undead.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-unequal.rock: $(PUZZLES_OBJDIR)/unequal.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-unruly.rock: $(PUZZLES_OBJDIR)/unruly.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
$(PUZZLES_OBJDIR)/sgt-untangle.rock: $(PUZZLES_OBJDIR)/untangle.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
endif

# special pattern rule for compiling puzzles with extra flags
$(PUZZLES_OBJDIR)/%.o: $(PUZZLES_SRCDIR)/%.c $(PUZZLES_SRCDIR)/puzzles.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PUZZLESFLAGS) -c $< -o $@
