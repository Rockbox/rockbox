#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

DUCKYSRCDIR := $(APPSDIR)/plugins/ducky
DUCKYBUILDDIR := $(BUILDDIR)/apps/plugins/ducky

ROCKS += $(DUCKYBUILDDIR)/ducky.rock

DUCKY_SRC := $(call preprocess, $(DUCKYSRCDIR)/SOURCES)
DUCKY_OBJ := $(call c2obj, $(DUCKY_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(DUCKY_SRC)

DUCKYFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O2

$(DUCKYBUILDDIR)/ducky.rock: $(DUCKY_OBJ) $(TLSFLIB)

$(DUCKYBUILDDIR)/%.o: $(DUCKYSRCDIR)/%.c $(DUCKYSRCDIR)/ducky.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(DUCKYFLAGS) -c $< -o $@
