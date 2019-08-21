#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

2600BOXSRCDIR := $(APPSDIR)/plugins/2600box
2600BOXBUILDDIR := $(BUILDDIR)/apps/plugins/2600box

ROCKS += $(2600BOXBUILDDIR)/2600box.rock

2600BOX_SRC := $(call preprocess, $(2600BOXSRCDIR)/SOURCES)
2600BOX_OBJ := $(call c2obj, $(2600BOX_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(2600BOX_SRC)

2600BOXFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O2


$(2600BOXBUILDDIR)/2600box.rock: $(2600BOX_OBJ)

$(2600BOXBUILDDIR)/%.o: $(2600BOXSRCDIR)/%.c $(2600BOXSRCDIR)/2600box.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(2600BOXFLAGS) -c $< -o $@
# output assembler file to look for optimations
#	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(2600BOXFLAGS) -S -fverbose-asm $< -o $@
