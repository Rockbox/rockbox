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

SDL_OBJ := $(call c2obj, $(SDL_SRC))
DUKE3D_OBJ = $(call c2obj, $(DUKE3D_SRC))
WOLF3D_OBJ = $(call c2obj, $(WOLF3D_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(SDL_SRC) $(DUKE3D_SRC) $(WOLF3D_SRC)
OTHER_INC += -I$(SDL_SRCDIR)/include

# include comes first because of possible system SDL headers taking
# precedence
SDLFLAGS = -I$(SDL_SRCDIR)/include $(filter-out -O%,$(PLUGINFLAGS))	\
-O3 -Wno-unused-parameter -Xpreprocessor -Wno-undef -Wcast-align -w

ifeq ($(ARCH_VERSION),6)
    SDLFLAGS += -mfloat-abi=softfp
endif

ifndef APP_TYPE
    ### no target has a big enough plugin buffer
    ROCKS += $(SDL_OBJDIR)/duke3d.ovl $(SDL_OBJDIR)/wolf3d.ovl
    DUKE3D_OUTLDS = $(SDL_OBJDIR)/duke3d.link
    WOLF3D_OUTLDS = $(SDL_OBJDIR)/wolf3d.link
    SDL_OVLFLAGS = -Wl,--gc-sections -Wl,-Map,$(basename $@).map
else
    ### simulator
    ROCKS += $(SDL_OBJDIR)/duke3d.rock
    ROCKS += $(SDL_OBJDIR)/wolf3d.rock
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

# common

$(SDL_OBJDIR)/%.o: $(SDL_SRCDIR)/%.c $(SDL_SRCDIR)/sdl.make $(BUILDDIR)/sysfont.h
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(SDLFLAGS) -c $< -o $@
	$(SILENT)$(OC) --redefine-syms=$(SDL_SRCDIR)/redefines.txt $@
