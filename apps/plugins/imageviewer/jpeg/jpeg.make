#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

JPEGSRCDIR := $(IMGVSRCDIR)/jpeg
JPEGBUILDDIR := $(IMGVBUILDDIR)/jpeg

ifeq ($(APP_TYPE),psp-homebrew)
IMGV_PLUGIN_CRT0 := $(ROOTDIR)/apps/plugins/imageviewer/plugin_crt0_psp.o
endif

JPEG_SRC := $(call preprocess, $(JPEGSRCDIR)/SOURCES)
JPEG_OBJ := $(call c2obj, $(JPEG_SRC)) $(IMGV_PLUGIN_CRT0)

OTHER_SRC += $(JPEG_SRC)

ROCKS += $(JPEGBUILDDIR)/jpeg.ovl

$(JPEGBUILDDIR)/jpeg.refmap: $(JPEG_OBJ)
$(JPEGBUILDDIR)/jpeg.link: $(PLUGIN_LDS) $(JPEGBUILDDIR)/jpeg.refmap
$(JPEGBUILDDIR)/jpeg.ovl: $(JPEG_OBJ)

# special pattern rule for compiling image decoder with extra flags
$(JPEGBUILDDIR)/%.o: $(JPEGSRCDIR)/%.c $(JPEGSRCDIR)/jpeg.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(IMGDECFLAGS) -c $< -o $@
