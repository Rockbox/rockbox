#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

BMPSRCDIR := $(IMGVSRCDIR)/bmp
BMPBUILDDIR := $(IMGVBUILDDIR)/bmp

ROCKS += $(BMPBUILDDIR)/bmp.rock

BMP_SRC := $(call preprocess, $(BMPSRCDIR)/SOURCES)
BMP_OBJ := $(call c2obj, $(BMP_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(BMP_SRC)

$(BMPBUILDDIR)/bmp.rock: $(BMP_OBJ)
