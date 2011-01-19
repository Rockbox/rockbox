#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

PPMSRCDIR := $(IMGVSRCDIR)/ppm
PPMBUILDDIR := $(IMGVBUILDDIR)/ppm

PPM_SRC := $(call preprocess, $(PPMSRCDIR)/SOURCES)
PPM_OBJ := $(call c2obj, $(PPM_SRC))

OTHER_SRC += $(PPM_SRC)

ROCKS += $(PPMBUILDDIR)/ppm.ovl

$(PPMBUILDDIR)/ppm.refmap: $(PPM_OBJ)
$(PPMBUILDDIR)/ppm.link: $(PLUGIN_LDS) $(PPMBUILDDIR)/ppm.refmap
$(PPMBUILDDIR)/ppm.ovl: $(PPM_OBJ)

# special pattern rule for compiling image decoder with extra flags
$(PPMBUILDDIR)/%.o: $(PPMSRCDIR)/%.c $(PPMSRCDIR)/ppm.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(IMGDECFLAGS) -c $< -o $@
