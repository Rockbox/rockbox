#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

CLOCKSRCDIR := $(APPSDIR)/plugins/clock
CLOCKBUILDDIR := $(BUILDDIR)/apps/plugins/clock

ROCKS += $(CLOCKBUILDDIR)/clock.rock

CLOCK_SRC := $(call preprocess, $(CLOCKSRCDIR)/SOURCES)
CLOCK_OBJ := $(call c2obj, $(CLOCK_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(CLOCK_SRC)

$(CLOCKBUILDDIR)/clock.rock: $(CLOCK_OBJ)
