#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

SDLSRCDIR := $(APPSDIR)/plugins/sdl
SDLBUILDDIR := $(BUILDDIR)/apps/plugins/sdl

ROCKS += $(SDLBUILDDIR)/sdl.rock

SDL_SRC := $(call preprocess, $(SDLSRCDIR)/SOURCES)
SDL_OBJ := $(call c2obj, $(SDL_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(SDL_SRC)

# include comes first because of possible system SDL headers taking
# precedence
SDLFLAGS = -I$(SDLSRCDIR)/include $(filter-out -O%,$(PLUGINFLAGS)) -O2 \
	-Wno-unused-parameter -Xpreprocessor -Wno-undef

$(SDLBUILDDIR)/sdl.rock: $(SDL_OBJ) $(TLSFLIB)

$(SDLBUILDDIR)/%.o: $(SDLSRCDIR)/%.c $(SDLSRCDIR)/sdl.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(SDLFLAGS) -c $< -o $@
