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

PUZZLES_SHARED_SRC = $(call preprocess, $(PUZZLES_SRCDIR)/SOURCES)
PUZZLES_SHARED_OBJ = $(call c2obj, $(PUZZLES_SHARED_SRC))

PUZZLES_GAMES_SRC = $(call preprocess, $(PUZZLES_SRCDIR)/SOURCES.games)
PUZZLES_GAMES_OBJ = $(call c2obj, $(PUZZLES_GAMES_SRC))

PUZZLES_HELP_SRC = $(wildcard $(PUZZLES_SRCDIR)/help/*)
PUZZLES_HELP_OBJ = $(call c2obj, $(PUZZLES_HELP_OBJ))

PUZZLES_SRC = $(PUZZLES_GAMES_SRC) $(PUZZLES_SHARED_SRC) $(PUZZLES_HELP_SRC)
PUZZLES_OBJ = $(call c2obj, $(PUZZLES_SRC))

PUZZLES_ROCKS = $(addprefix $(PUZZLES_OBJDIR)/sgt-, $(notdir $(PUZZLES_GAMES_SRC:.c=.rock)))

OTHER_SRC += $(PUZZLES_SRC)
ROCKS += $(PUZZLES_ROCKS)

PUZZLES_OPTIMIZE = -O2

ifeq ($(MODELNAME), sansac200v2)
PUZZLES_OPTIMIZE = -Os # tiny plugin buffer
endif

# we suppress all warnings with -w
PUZZLESFLAGS = -I$(PUZZLES_SRCDIR)/dummy $(filter-out			\
		-O%,$(PLUGINFLAGS)) $(PUZZLES_OPTIMIZE)			\
		-Wno-unused-parameter -Wno-sign-compare			\
		-Wno-strict-aliasing -DFOR_REAL				\
		-I$(PUZZLES_SRCDIR)/src -I$(PUZZLES_SRCDIR) -include	\
		$(PUZZLES_SRCDIR)/rbcompat.h -ffunction-sections	\
		-fdata-sections -w -Wl,--gc-sections

$(PUZZLES_OBJDIR)/sgt-%.rock: $(PUZZLES_OBJDIR)/src/%.o $(PUZZLES_OBJDIR)/help/%.o $(PUZZLES_SHARED_OBJ) $(TLSFLIB)
	$(call PRINTS,LD $(@F))$(CC) $(PLUGINFLAGS) -o $(PUZZLES_OBJDIR)/$*.elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(filter-out -Wl%.map, $(PLUGINLDFLAGS)) -Wl,-Map,$(PUZZLES_OBJDIR)/src/$*.map
	$(SILENT)$(call objcopy,$(PUZZLES_OBJDIR)/$*.elf,$@)

$(PUZZLES_SRCDIR)/rbcompat.h:	$(APPSDIR)/plugin.h			\
				$(APPSDIR)/plugins/lib/pluginlib_exit.h	\
				$(BUILDDIR)/sysfont.h			\
				$(PUZZLES_SRCDIR)/rbassert.h		\
				$(TLSFLIB_DIR)/src/tlsf.h \
				$(BUILDDIR)/lang_enum.h

# special pattern rule for compiling puzzles with extra flags
$(PUZZLES_OBJDIR)/%.o: $(PUZZLES_SRCDIR)/%.c $(PUZZLES_SRCDIR)/puzzles.make $(PUZZLES_SRCDIR)/rbcompat.h
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PUZZLESFLAGS) -c $< -o $@
