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

BMP_SRC := $(call preprocess, $(BMPSRCDIR)/SOURCES)
BMP_OBJ := $(call c2obj, $(BMP_SRC))

OTHER_SRC += $(BMP_SRC)

ROCKS += $(BMPBUILDDIR)/bmp.ovl

$(BMPBUILDDIR)/bmp.refmap: $(BMP_OBJ)
$(BMPBUILDDIR)/bmp.link: $(PLUGIN_LDS) $(BMPBUILDDIR)/bmp.refmap
$(BMPBUILDDIR)/bmp.ovl: $(BMP_OBJ)

# special pattern rule for compiling image decoder with extra flags
$(BMPBUILDDIR)/%.o: $(BMPSRCDIR)/%.c $(BMPSRCDIR)/bmp.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(IMGDECFLAGS) -c $< -o $@
