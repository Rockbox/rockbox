#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: $
#

WIKIVIEWER_SRCDIR := $(APPSDIR)/plugins/wikiviewer
WIKIVIEWER_BUILDDIR := $(BUILDDIR)/apps/plugins/wikiviewer

ROCKS += $(WIKIVIEWER_BUILDDIR)/wikiviewer.rock

WIKIVIEWER_SRC := $(call preprocess, $(WIKIVIEWER_SRCDIR)/SOURCES)
WIKIVIEWER_OBJ := $(call c2obj, $(WIKIVIEWER_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(WIKIVIEWER_SRC)

$(WIKIVIEWER_BUILDDIR)/wikiviewer.rock: $(WIKIVIEWER_OBJ)
