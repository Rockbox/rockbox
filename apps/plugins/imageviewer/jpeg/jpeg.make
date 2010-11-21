#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

JPEGSRCDIR := $(IMGVSRCDIR)/jpeg
JPEGBUILDDIR := $(IMGVBUILDDIR)/jpeg

JPEG_SRC := $(call preprocess, $(JPEGSRCDIR)/SOURCES)
JPEG_OBJ := $(call c2obj, $(JPEG_SRC))

OTHER_SRC += $(JPEG_SRC)

ROCKS += $(JPEGBUILDDIR)/jpeg.ovl

$(JPEGBUILDDIR)/jpeg.refmap: $(JPEG_OBJ)
$(JPEGBUILDDIR)/jpeg.link: $(PLUGIN_LDS) $(JPEGBUILDDIR)/jpeg.refmap
$(JPEGBUILDDIR)/jpeg.ovl: $(JPEG_OBJ)

# special pattern rule for compiling image decoder with extra flags
$(JPEGBUILDDIR)/%.o: $(JPEGSRCDIR)/%.c $(JPEGSRCDIR)/jpeg.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(IMGDECFLAGS) -c $< -o $@
