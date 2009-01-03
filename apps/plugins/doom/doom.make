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

DOOMCFLAGS = $(PLUGINFLAGS) -Wno-strict-prototypes

# The arm code compiles too large for the plugin buffer when set to optimize for speed
ifeq ($(CPU), arm)
    DOOMCFLAGS += -Os
ifndef SIMVER
ifeq ($(TARGET), IRIVER_H100)
    DOOMCCFLAGS += -mstructure-size-boundary=8
endif
endif
else
    DOOMCFLAGS += -O2
endif

$(DOOMBUILDDIR)/sscanf.c: $(FIRMDIR)/common/sscanf.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CP $<)cp $< $@

$(DOOMBUILDDIR)/sscanf.o: $(DOOMBUILDDIR)/sscanf.c

$(DOOMBUILDDIR)/doom.rock: $(DOOM_OBJ)
# for some reason, this doesn't match the implicit rule in plugins.make,
# so we have to duplicate the link command here
	$(call PRINTS,LD $(@F))
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $*.elf \
		$(filter %.o, $^) \
		$(filter %.a, $^) \
		-lgcc $(PLUGINLDFLAGS)
ifdef SIMVER
	$(SILENT)cp $*.elf $@
else
	$(SILENT)$(OC) -O binary $*.elf $@
endif

# new rule needed to use extra compile flags
$(DOOMBUILDDIR)/%.o: $(DOOMSRCDIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(DOOMCFLAGS) -c $< -o $@
