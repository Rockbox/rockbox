#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

WIKIPEDIASRCDIR := $(APPSDIR)/plugins/Wikipedia
WIKIPEDIABUILDDIR := $(BUILDDIR)/apps/plugins/Wikipedia

ROCKS += $(WIKIPEDIABUILDDIR)/Wikipedia.rock

WIKIPEDIA_SRC := $(call preprocess, $(WIKIPEDIASRCDIR)/SOURCES)
WIKIPEDIA_OBJ := $(call c2obj, $(WIKIPEDIA_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(WIKIPEDIA_SRC)

$(WIKIPEDIABUILDDIR)/Wikipedia.rock: $(WIKIPEDIA_OBJ)
