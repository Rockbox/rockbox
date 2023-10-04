#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

TCPLUG_SRCDIR := $(APPSDIR)/plugins/tagcache
TCPLUG_BUILDDIR := $(BUILDDIR)/apps/plugins/tagcache

ROCKS += $(TCPLUG_BUILDDIR)/db_commit.rock

TCPLUG_FLAGS = $(PLUGINFLAGS) -fno-strict-aliasing -Wno-unused \
				-I$(TCPLUG_SRCDIR) -ffunction-sections	       \
				-fdata-sections -Wl,--gc-sections
TCPLUG_SRC := $(call preprocess, $(TCPLUG_SRCDIR)/SOURCES)
TCPLUG_OBJ := $(call c2obj, $(TCPLUG_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(APPSDIR)/tagcache.c $(TCPLUG_SRC)

$(TCPLUG_BUILDDIR)/db_commit.rock: $(TCPLUG_OBJ)

# special pattern rule for compiling with extra flags
$(TCPLUG_BUILDDIR)/%.o: $(TCPLUG_SRCDIR)/%.c $(TCPLUG_SRCDIR)/tagcache.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(TCPLUG_FLAGS) -c $< -o $@

