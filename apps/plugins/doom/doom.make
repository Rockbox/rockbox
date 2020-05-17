#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

DOOMSRCDIR := $(APPSDIR)/plugins/doom
DOOMBUILDDIR := $(BUILDDIR)/apps/plugins/doom

ROCKS += $(DOOMBUILDDIR)/doom.rock

DOOM_SRC := $(call preprocess, $(DOOMSRCDIR)/SOURCES)
DOOM_OBJ := $(call c2obj, $(DOOM_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(DOOM_SRC)

DOOMCFLAGS = $(PLUGINFLAGS) -Wno-strict-prototypes -O2 -fno-strict-aliasing

# Set '-fgnu89-inline' if supported (GCCVER >= 4.1.3, GCCNUM > 401)
ifeq ($(shell expr $(GCCNUM) \> 401),1)
    DOOMCFLAGS += -fgnu89-inline
endif
# Disable stringop-truncation warnings on GCC 8 or greater
ifeq ($(shell expr $(GCCNUM) \> 800),1)
    DOOMCFLAGS += -Wno-stringop-truncation
endif

ifndef APP_TYPE
ifeq ($(TARGET), IRIVER_H100)
    DOOMCCFLAGS += -mstructure-size-boundary=8
endif
endif

$(DOOMBUILDDIR)/doom.rock: $(DOOM_OBJ)

# new rule needed to use extra compile flags
$(DOOMBUILDDIR)/%.o: $(DOOMSRCDIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(DOOMCFLAGS) -c $< -o $@
