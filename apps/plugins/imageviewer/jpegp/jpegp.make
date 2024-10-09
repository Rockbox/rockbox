#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

JPEGPSRCDIR := $(IMGVSRCDIR)/jpegp
JPEGPBUILDDIR := $(IMGVBUILDDIR)/jpegp

JPEGP_SRC := $(call preprocess, $(JPEGPSRCDIR)/SOURCES)
JPEGP_OBJ := $(call c2obj, $(JPEGP_SRC))

OTHER_SRC += $(JPEGP_SRC)

ROCKS += $(JPEGPBUILDDIR)/jpegp.ovl

$(JPEGPBUILDDIR)/jpegp.refmap: $(JPEGP_OBJ) $(TLSFLIB)
$(JPEGPBUILDDIR)/jpegp.link: $(PLUGIN_LDS) $(JPEGPBUILDDIR)/jpegp.refmap
$(JPEGPBUILDDIR)/jpegp.ovl: $(JPEGP_OBJ) $(TLSFLIB)

JPEGPFLAGS = $(IMGDECFLAGS)
ifndef DEBUG
JPEGPFLAGS += -Os
endif

# Compile plugin with extra flags (adapted from ZXBox)
$(JPEGPBUILDDIR)/%.o: $(JPEGPSRCDIR)/%.c $(JPEGPSRCDIR)/jpegp.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(JPEGPFLAGS) -c $< -o $@
