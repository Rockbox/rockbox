#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

FROTZSRCDIR := $(APPSDIR)/plugins/frotz
FROTZBUILDDIR := $(BUILDDIR)/apps/plugins/frotz

ROCKS += $(FROTZBUILDDIR)/frotz.rock

FROTZ_SRC := $(call preprocess, $(FROTZSRCDIR)/SOURCES)
FROTZ_OBJ := $(call c2obj, $(FROTZ_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(FROTZ_SRC)

$(FROTZBUILDDIR)/frotz.rock: $(FROTZ_OBJ)
