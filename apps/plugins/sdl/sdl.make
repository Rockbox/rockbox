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
TESTSDL_SRC := $(call preprocess, $(SDL_SRCDIR)/SOURCES.test_sdl)

SDL_OBJ := $(call c2obj, $(SDL_SRC))
DUKE3D_OBJ = $(call c2obj, $(DUKE3D_SRC))
TESTSDL_OBJ = $(call c2obj, $(TESTSDL_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(SDL_SRC) $(DUKE3D_SRC) $(TESTSDL_SRC)
OTHER_INC += -I$(SDL_SRCDIR)/include

# include comes first because of possible system SDL headers taking
# precedence
SDLFLAGS = -I$(SDL_SRCDIR)/include $(filter-out -O%,$(PLUGINFLAGS))	\
-O3 -Wno-unused-parameter -Xpreprocessor -Wno-undef -Wcast-align -w -save-temps

ifeq ($(ARCH_VERSION),6)
    SDLFLAGS += -mfloat-abi=softfp
endif

ifndef APP_TYPE
    ### no target has a big enough plugin buffer
    ROCKS += $(SDL_OBJDIR)/duke3d.ovl
    ROCKS += $(SDL_OBJDIR)/test_sdl.ovl
    DUKE3D_OUTLDS = $(SDL_OBJDIR)/duke3d.link
    TESTSDL_OUTLDS = $(SDL_OBJDIR)/test_sdl.link
    DUKE3D_OVLFLAGS = -T$(DUKE3D_OUTLDS) -Wl,--gc-sections -Wl,-Map,$(basename $@).map
    TESTSDL_OVLFLAGS = -T$(TESTSDL_OUTLDS) -Wl,--gc-sections -Wl,-Map,$(basename $@).map
else
    ### simulator
    ROCKS += $(SDL_OBJDIR)/duke3d.rock
    ROCKS += $(SDL_OBJDIR)/test_sdl.rock
endif

$(SDL_OBJDIR)/duke3d.rock: $(SDL_OBJ) $(DUKE3D_OBJ) $(TLSFLIB)

$(SDL_OBJDIR)/duke3d.refmap: $(SDL_OBJ) $(DUKE3D_OBJ) $(TLSFLIB)

$(DUKE3D_OUTLDS): $(PLUGIN_LDS) $(SDL_OBJDIR)/duke3d.refmap
	$(call PRINTS,PP $(@F))$(call preprocess2file,$<,$@,-DOVERLAY_OFFSET=$(shell \
		$(TOOLSDIR)/ovl_offset.pl $(SDL_OBJDIR)/duke3d.refmap))

$(SDL_OBJDIR)/duke3d.ovl: $(SDL_OBJ) $(DUKE3D_OBJ) $(TLSFLIB) $(DUKE3D_OUTLDS)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(DUKE3D_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(call objcopy,$(basename $@).elf,$@)

###

$(SDL_OBJDIR)/test_sdl.rock: $(SDL_OBJ) $(TESTSDL_OBJ) $(TLSFLIB)

$(SDL_OBJDIR)/test_sdl.refmap: $(SDL_OBJ) $(TESTSDL_OBJ) $(TLSFLIB)

$(TESTSDL_OUTLDS): $(PLUGIN_LDS) $(SDL_OBJDIR)/test_sdl.refmap
	$(call PRINTS,PP $(@F))$(call preprocess2file,$<,$@,-DOVERLAY_OFFSET=$(shell \
		$(TOOLSDIR)/ovl_offset.pl $(SDL_OBJDIR)/test_sdl.refmap))

$(SDL_OBJDIR)/test_sdl.ovl: $(SDL_OBJ) $(TESTSDL_OBJ) $(TLSFLIB) $(TESTSDL_OUTLDS)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(TESTSDL_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(call objcopy,$(basename $@).elf,$@)

###

$(SDL_OBJDIR)/%.o: $(SDL_SRCDIR)/%.c $(SDL_SRCDIR)/sdl.make $(BUILDDIR)/sysfont.h
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(SDLFLAGS) -c $< -o $@
	$(SILENT)$(OC) --redefine-syms=$(SDL_SRCDIR)/redefines.txt $@
