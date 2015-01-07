#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

XRACERSRCDIR := $(APPSDIR)/plugins/xracer
XRACERBUILDDIR := $(BUILDDIR)/apps/plugins/xracer

ROCKS += $(XRACERBUILDDIR)/xracer.rock

XRACER_SRC := $(call preprocess, $(XRACERSRCDIR)/SOURCES)
XRACER_OBJ := $(call c2obj, $(XRACER_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(XRACER_SRC)

XRACERFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O2

$(XRACERBUILDDIR)/xracer.rock: $(XRACER_OBJ)

$(XRACERBUILDDIR)/%.o: $(XRACERSRCDIR)/%.c $(XRACERSRCDIR)/%.h $(XRACERSRCDIR)/xracer.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(XRACERFLAGS) -c $< -o $@
