#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

PHYSICSFRACTALSSRCDIR := $(APPSDIR)/plugins/physicsfractals
PHYSICSFRACTALSBUILDDIR := $(BUILDDIR)/apps/plugins/physicsfractals

ROCKS += $(PHYSICSFRACTALSBUILDDIR)/physicsfractals.rock

PHYSICSFRACTALS_SRC := $(call preprocess, $(PHYSICSFRACTALSSRCDIR)/SOURCES)
PHYSICSFRACTALS_OBJ := $(call c2obj, $(PHYSICSFRACTALS_SRC))

# add source files to OTHERSRC to get automatic dependencies
OTHER_SRC += $(PHYSICSFRACTALS_SRC)

$(PHYSICSFRACTALSBUILDDIR)/physicsfractals.rock: $(PHYSICSFRACTALS_OBJ)
