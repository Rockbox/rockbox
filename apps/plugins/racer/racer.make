#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

RACERSRCDIR := $(APPSDIR)/plugins/racer
RACERBUILDDIR := $(BUILDDIR)/apps/plugins/racer

ROCKS += $(RACERBUILDDIR)/racer.rock

RACER_SRC := $(call preprocess, $(RACERSRCDIR)/SOURCES)
RACER_OBJ := $(call c2obj, $(RACER_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(RACER_SRC)

RACERFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O2

$(RACERBUILDDIR)/racer.rock: $(RACER_OBJ)

$(RACERBUILDDIR)/%.o: $(RACERSRCDIR)/%.c $(RACERSRCDIR)/racer.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(RACERFLAGS) -c $< -o $@
