#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

SDL2_SRCDIR := $(APPSDIR)/plugins/sdl2
SDL2_OBJDIR := $(BUILDDIR)/apps/plugins/sdl2

SDL2_SRC := $(call preprocess, $(SDL2_SRCDIR)/SOURCES)
TEST_SDL2_SRC := $(call preprocess, $(SDL2_SRCDIR)/SOURCES.test_sdl2)

SDL2_OBJ := $(call c2obj, $(SDL2_SRC))
TEST_SDL2_OBJ := $(call c2obj, $(TEST_SDL2_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(SDL2_SRC) $(TEST_SDL2_SRC)
OTHER_INC += -I$(SDL2_SRCDIR)/lib/SDL2/include

# include comes first because of possible system SDL2 headers taking
# precedence
# some of these are for Quake only
SDL2FLAGS = -I$(SDL2_SRCDIR)/lib/SDL2/include $(filter-out	\
-O%,$(PLUGINFLAGS)) -O3 -Wno-unused-parameter -Xpreprocessor	\
-Wno-undef -Wcast-align -ffast-math -funroll-loops		\
-fomit-frame-pointer -fexpensive-optimizations -D_GNU_SOURCE=1	\
-D_REENTRANT -DSDL -DELF # disable all warnings

# WIP SDL2FLAGS for warning deletions
#SDL2FLAGS = -I$(SDL2_SRCDIR)/include $(filter-out -O%,$(PLUGINFLAGS))		\
#-O3 -Wno-unused-parameter -Xpreprocessor -Wno-undef -Wno-sign-compare \
#-Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-unknown-pragmas \
#-ffast-math -funroll-loops -fomit-frame-pointer -fexpensive-optimizations	\
#-D_GNU_SOURCE=1 -D_REENTRANT -DSDL2 -DELF

# use FPU on ARMv6
ifeq ($(ARCH_VERSION),6)
    SDL2FLAGS += -mfloat-abi=softfp
endif

ifndef APP_TYPE
    ### no target has a big enough plugin buffer
    ROCKS += $(SDL2_OBJDIR)/test_sdl2.ovl

    TEST_SDL2_OUTLDS = $(SDL2_OBJDIR)/test_sdl2.link

    SDL2_OVLFLAGS = -Wl,--gc-sections -Wl,-Map,$(basename $@).map
else
    ### simulator
    ROCKS += $(SDL2_OBJDIR)/test_sdl2.rock
endif

# Duke3D

$(SDL2_OBJDIR)/test_sdl2.rock: $(SDL2_OBJ) $(TEST_SDL2_OBJ) $(TLSFLIB)

$(SDL2_OBJDIR)/test_sdl2.refmap: $(SDL2_OBJ) $(TEST_SDL2_OBJ) $(TLSFLIB)

$(TEST_SDL2_OUTLDS): $(PLUGIN_LDS) $(SDL2_OBJDIR)/test_sdl2.refmap
	$(call PRINTS,PP $(@F))$(call preprocess2file,$<,$@,-DOVERLAY_OFFSET=$(shell \
		$(TOOLSDIR)/ovl_offset.pl $(SDL2_OBJDIR)/test_sdl2.refmap))

$(SDL2_OBJDIR)/test_sdl2.ovl: $(SDL2_OBJ) $(TEST_SDL2_OBJ) $(TLSFLIB) $(TEST_SDL2_OUTLDS)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc -T$(TEST_SDL2_OUTLDS) $(SDL2_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(call objcopy,$(basename $@).elf,$@)


# common

$(SDL2_OBJDIR)/%.o: $(SDL2_SRCDIR)/%.c $(SDL2_SRCDIR)/sdl2.make $(BUILDDIR)/sysfont.h
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(call full_path_subst,$(ROOTDIR)/%,%,$<))$(CC) -I$(dir $<) $(SDL2FLAGS) -c $< -o $@
	$(SILENT)$(OC) --redefine-syms=$(SDL2_SRCDIR)/redefines.txt $@
