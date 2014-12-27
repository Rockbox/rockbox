#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

UNITCONVSRCDIR := $(APPSDIR)/plugins/unitconv
UNITCONVBUILDDIR := $(BUILDDIR)/apps/plugins/unitconv

ROCKS += $(UNITCONVBUILDDIR)/unitconv.rock

UNITCONV_SRC := $(call preprocess, $(UNITCONVSRCDIR)/SOURCES)
UNITCONV_OBJ := $(call c2obj, $(UNITCONV_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(UNITCONV_SRC)

$(UNITCONVBUILDDIR)/unitconv.rock: $(UNITCONV_OBJ)
