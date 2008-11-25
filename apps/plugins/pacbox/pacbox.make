#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

PACBOXSRCDIR := $(APPSDIR)/plugins/pacbox
PACBOXBUILDDIR := $(BUILDDIR)/apps/plugins/pacbox

ROCKS += $(PACBOXBUILDDIR)/pacbox.rock

PACBOX_SRC := $(call preprocess, $(PACBOXSRCDIR)/SOURCES)
PACBOX_OBJ := $(call c2obj, $(PACBOX_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(PACBOX_SRC)

PACBOXFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O2

$(PACBOXBUILDDIR)/pacbox.rock: $(PACBOX_OBJ)

$(PACBOXBUILDDIR)/%.o: $(PACBOXSRCDIR)/%.c $(PLUGINBITMAPLIB) $(PACBOXSRCDIR)/pacbox.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PACBOXFLAGS) -c $< -o $@
