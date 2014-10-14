#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

XWORLDSRCDIR := $(APPSDIR)/plugins/xworld
XWORLDBUILDDIR := $(BUILDDIR)/apps/plugins/xworld

ROCKS += $(XWORLDBUILDDIR)/xworld.rock

XWORLD_SRC := $(call preprocess, $(XWORLDSRCDIR)/SOURCES)
XWORLD_OBJ := $(call c2obj, $(XWORLD_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(XWORLD_SRC)

XWORLDFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O2

$(XWORLDBUILDDIR)/xworld.rock: $(XWORLD_OBJ)

$(XWORLDBUILDDIR)/%.o: $(XWORLDSRCDIR)/%.c $(XWORLDSRCDIR)/xworld.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(XWORLDFLAGS) -c $< -o $@
