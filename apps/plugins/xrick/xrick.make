#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

XRICKSRCDIR := $(APPSDIR)/plugins/xrick
XRICKBUILDDIR := $(BUILDDIR)/apps/plugins/xrick

INCLUDES += -I$(XRICKSRCDIR)../ \
            -I$(XRICKSRCDIR)/3rd_party

ROCKS += $(XRICKBUILDDIR)/xrick.rock

XRICK_SRC := $(call preprocess, $(XRICKSRCDIR)/SOURCES)
XRICK_OBJ := $(call c2obj, $(XRICK_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(XRICK_SRC)

XRICKCFLAGS = $(PLUGINFLAGS) -std=gnu99 -O2

$(XRICKBUILDDIR)/xrick.rock: $(XRICK_OBJ)

# new rule needed to use extra compile flags
$(XRICKBUILDDIR)/%.o: $(XRICKSRCDIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(XRICKCFLAGS) -c $< -o $@
