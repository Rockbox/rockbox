#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

PNGSRCDIR := $(IMGVSRCDIR)/png
PNGBUILDDIR := $(IMGVBUILDDIR)/png

ROCKS += $(PNGBUILDDIR)/png.rock

PNG_SRC := $(call preprocess, $(PNGSRCDIR)/SOURCES)
PNG_OBJ := $(call c2obj, $(PNG_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(PNG_SRC)

# Use -O3 for png plugin : it gives a bigger file but very good performances
PNGFLAGS = $(PLUGINFLAGS) -O3 -DNO_GZCOMPRESS -DNO_GZIP

$(PNGBUILDDIR)/png.rock: $(PNG_OBJ)

# Compile PNG plugin with extra flags (adapted from ZXBox)
$(PNGBUILDDIR)/%.o: $(PNGSRCDIR)/%.c $(PNGSRCDIR)/png.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PNGFLAGS) -c $< -o $@
