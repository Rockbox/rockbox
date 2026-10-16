#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

PNGSRCDIR := $(IMGVSRCDIR)/png
PNGBUILDDIR := $(IMGVBUILDDIR)/png

ifeq ($(APP_TYPE),psp-homebrew)
IMGV_PLUGIN_CRT0 := $(ROOTDIR)/apps/plugins/imageviewer/plugin_crt0_psp.o
endif

PNG_SRC := $(call preprocess, $(PNGSRCDIR)/SOURCES)
PNG_OBJ := $(call c2obj, $(PNG_SRC)) $(IMGV_PLUGIN_CRT0)

OTHER_SRC += $(PNG_SRC)

ROCKS += $(PNGBUILDDIR)/png.ovl

$(PNGBUILDDIR)/png.refmap: $(PNG_OBJ)
$(PNGBUILDDIR)/png.link: $(PLUGIN_LDS) $(PNGBUILDDIR)/png.refmap
$(PNGBUILDDIR)/png.ovl: $(PNG_OBJ)

# Use -O3 for png plugin : it gives a bigger file but very good performances
PNGFLAGS = $(IMGDECFLAGS) -Os

# Compile PNG plugin with extra flags (adapted from ZXBox)
$(PNGBUILDDIR)/%.o: $(PNGSRCDIR)/%.c $(PNGSRCDIR)/png.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PNGFLAGS) -c $< -o $@
