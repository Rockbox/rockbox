#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

PDBOXSRCDIR := $(APPSDIR)/plugins/pdbox
PDBOXBUILDDIR := $(BUILDDIR)/apps/plugins/pdbox

ROCKS += $(PDBOXBUILDDIR)/pdbox.rock

PDBOXSRC := $(call preprocess, $(PDBOXSRCDIR)/SOURCES)
PDBOXOBJ := $(call c2obj, $(PDBOXSRC))

# add source files to OTHERSRC to get automatic dependencies
OTHERSRC += $(PDBOXSRC)

$(PDBOXBUILDDIR)/pdbox.rock: $(PDBOXOBJ)

PDBOXFLAGS = $(PLUGINFLAGS) \
             -DFIXEDPOINT -DSTATIC -DPD \
             -I$(PDBOXSRCDIR) -I$(PDBOXSRCDIR)/PDa/src \
             -DBMALLOC -I$(PDBOXSRCDIR)/dbestfit-3.3

# Compile PDBox with extra flags (adapted from ZXBox)
$(PDBOXBUILDDIR)/%.o: $(PDBOXSRCDIR)/%.c $(PDBOXSRCDIR)/pdbox.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PDBOXFLAGS) -c $< -o $@

