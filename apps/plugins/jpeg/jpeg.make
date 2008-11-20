#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 19082 2008-11-10 23:54:24Z zagor $
#

JPEGSRCDIR := $(APPSDIR)/plugins/jpeg
JPEGBUILDDIR := $(BUILDDIR)/apps/plugins/jpeg

ROCKS += $(JPEGBUILDDIR)/jpeg.rock

JPEG_SRC := $(call preprocess, $(JPEGSRCDIR)/SOURCES)
JPEG_OBJ := $(call c2obj, $(JPEG_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(JPEG_SRC)

$(JPEGBUILDDIR)/jpeg.rock: $(JPEG_OBJ)
