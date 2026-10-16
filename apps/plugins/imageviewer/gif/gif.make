#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

GIFSRCDIR := $(IMGVSRCDIR)/gif
GIFBUILDDIR := $(IMGVBUILDDIR)/gif

ifeq ($(APP_TYPE),psp-homebrew)
IMGV_PLUGIN_CRT0 := $(ROOTDIR)/apps/plugins/imageviewer/plugin_crt0_psp.o
endif

GIF_SRC := $(call preprocess, $(GIFSRCDIR)/SOURCES)
GIF_OBJ := $(call c2obj, $(GIF_SRC)) $(IMGV_PLUGIN_CRT0)

OTHER_SRC += $(GIF_SRC)

ROCKS += $(GIFBUILDDIR)/gif.ovl

$(GIFBUILDDIR)/gif.refmap: $(GIF_OBJ) $(TLSFLIB)
$(GIFBUILDDIR)/gif.link: $(PLUGIN_LDS) $(GIFBUILDDIR)/gif.refmap
$(GIFBUILDDIR)/gif.ovl: $(GIF_OBJ) $(TLSFLIB)

#-Os breaks decoder - dunno why
GIFFLAGS = $(IMGDECFLAGS) -O2

# Compile PNG plugin with extra flags (adapted from ZXBox)
$(GIFBUILDDIR)/%.o: $(GIFSRCDIR)/%.c $(GIFSRCDIR)/gif.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(GIFFLAGS) -c $< -o $@
