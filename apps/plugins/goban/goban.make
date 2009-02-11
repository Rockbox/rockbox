#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#


GOBANSRCDIR := $(APPSDIR)/plugins/goban
GOBANBUILDDIR := $(BUILDDIR)/apps/plugins/goban

ROCKS += $(GOBANBUILDDIR)/goban.rock


GOBAN_SRC := $(call preprocess, $(GOBANSRCDIR)/SOURCES)
GOBAN_OBJ := $(call c2obj, $(GOBAN_SRC))

OTHER_SRC += $(GOBAN_SRC)

$(GOBANBUILDDIR)/goban.rock: $(GOBAN_OBJ)
