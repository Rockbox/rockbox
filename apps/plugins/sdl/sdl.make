#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

SDL_SRCDIR := $(APPSDIR)/plugins/sdl
SDL_OBJDIR := $(BUILDDIR)/apps/plugins/sdl

SDL_SRC := $(call preprocess, $(SDL_SRCDIR)/SOURCES)
DUKE3D_SRC := $(call preprocess, $(SDL_SRCDIR)/SOURCES.duke)
WOLF3D_SRC := $(call preprocess, $(SDL_SRCDIR)/SOURCES.wolf)
QUAKE_SRC := $(call preprocess, $(SDL_SRCDIR)/SOURCES.quake)

SDL_OBJ := $(call c2obj, $(SDL_SRC))
DUKE3D_OBJ = $(call c2obj, $(DUKE3D_SRC))
WOLF3D_OBJ = $(call c2obj, $(WOLF3D_SRC))
QUAKE_OBJ = $(call c2obj, $(QUAKE_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(SDL_SRC) $(DUKE3D_SRC) $(WOLF3D_SRC) $(QUAKE_SRC)
OTHER_INC += -I$(SDL_SRCDIR)/include

# include comes first because of possible system SDL headers taking
# precedence
# some of these are for Quake only
SDLFLAGS = -I$(SDL_SRCDIR)/include $(filter-out -O%,$(PLUGINFLAGS))		\
-O3 -Wno-unused-parameter -Xpreprocessor -Wno-undef -Wcast-align	\
-ffast-math -funroll-loops -fomit-frame-pointer -fexpensive-optimizations	\
-D_GNU_SOURCE=1 -D_REENTRANT -DSDL -DELF -w # disable all warnings

# WIP SDLFLAGS for warning deletions
#SDLFLAGS = -I$(SDL_SRCDIR)/include $(filter-out -O%,$(PLUGINFLAGS))		\
#-O3 -Wno-unused-parameter -Xpreprocessor -Wno-undef -Wno-sign-compare \
#-Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-unknown-pragmas \
#-ffast-math -funroll-loops -fomit-frame-pointer -fexpensive-optimizations	\
#-D_GNU_SOURCE=1 -D_REENTRANT -DSDL -DELF

ifndef APP_TYPE
    ### no target has a big enough plugin buffer
    ROCKS += $(SDL_OBJDIR)/duke3d.ovl
    ROCKS += $(SDL_OBJDIR)/wolf3d.ovl
    ROCKS += $(SDL_OBJDIR)/quake.ovl

    DUKE3D_OUTLDS = $(SDL_OBJDIR)/duke3d.link
    WOLF3D_OUTLDS = $(SDL_OBJDIR)/wolf3d.link
    QUAKE_OUTLDS = $(SDL_OBJDIR)/quake.link

    SDL_OVLFLAGS = -Wl,--gc-sections -Wl,-Map,$(basename $@).map
else
    ### simulator
    ROCKS += $(SDL_OBJDIR)/duke3d.rock
    ROCKS += $(SDL_OBJDIR)/wolf3d.rock
    ROCKS += $(SDL_OBJDIR)/quake.rock
endif

# Duke3D

$(SDL_OBJDIR)/duke3d.rock: $(SDL_OBJ) $(DUKE3D_OBJ) $(TLSFLIB)

$(SDL_OBJDIR)/duke3d.refmap: $(SDL_OBJ) $(DUKE3D_OBJ) $(TLSFLIB)

$(DUKE3D_OUTLDS): $(PLUGIN_LDS) $(SDL_OBJDIR)/duke3d.refmap
	$(call PRINTS,PP $(@F))$(call preprocess2file,$<,$@,-DOVERLAY_OFFSET=$(shell \
		$(TOOLSDIR)/ovl_offset.pl $(SDL_OBJDIR)/duke3d.refmap))

$(SDL_OBJDIR)/duke3d.ovl: $(SDL_OBJ) $(DUKE3D_OBJ) $(TLSFLIB) $(DUKE3D_OUTLDS)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc -T$(DUKE3D_OUTLDS) $(SDL_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(call objcopy,$(basename $@).elf,$@)

# Wolf3D

$(SDL_OBJDIR)/wolf3d.rock: $(SDL_OBJ) $(WOLF3D_OBJ) $(TLSFLIB)

$(SDL_OBJDIR)/wolf3d.refmap: $(SDL_OBJ) $(WOLF3D_OBJ) $(TLSFLIB)

$(WOLF3D_OUTLDS): $(PLUGIN_LDS) $(SDL_OBJDIR)/wolf3d.refmap
	$(call PRINTS,PP $(@F))$(call preprocess2file,$<,$@,-DOVERLAY_OFFSET=$(shell \
		$(TOOLSDIR)/ovl_offset.pl $(SDL_OBJDIR)/wolf3d.refmap))

$(SDL_OBJDIR)/wolf3d.ovl: $(SDL_OBJ) $(WOLF3D_OBJ) $(TLSFLIB) $(WOLF3D_OUTLDS)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc  -T$(WOLF3D_OUTLDS) $(SDL_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(call objcopy,$(basename $@).elf,$@)

# Quake
###

$(SDL_OBJDIR)/quake.rock: $(SDL_OBJ) $(QUAKE_OBJ) $(TLSFLIB)

$(SDL_OBJDIR)/quake.refmap: $(SDL_OBJ) $(QUAKE_OBJ) $(TLSFLIB)

$(QUAKE_OUTLDS): $(PLUGIN_LDS) $(SDL_OBJDIR)/quake.refmap
	$(call PRINTS,PP $(@F))$(call preprocess2file,$<,$@,-DOVERLAY_OFFSET=$(shell \
		$(TOOLSDIR)/ovl_offset.pl $(SDL_OBJDIR)/quake.refmap))

$(SDL_OBJDIR)/quake.ovl: $(SDL_OBJ) $(QUAKE_OBJ) $(TLSFLIB) $(QUAKE_OUTLDS)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc -T$(QUAKE_OUTLDS) $(SDL_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(call objcopy,$(basename $@).elf,$@)

###

# common

$(SDL_OBJDIR)/%.o: $(SDL_SRCDIR)/%.c $(SDL_SRCDIR)/sdl.make $(BUILDDIR)/sysfont.h
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(call full_path_subst,$(ROOTDIR)/%,%,$<))$(CC) -I$(dir $<) $(SDLFLAGS) -c $< -o $@
	$(SILENT)$(OC) --redefine-syms=$(SDL_SRCDIR)/redefines.txt $@
