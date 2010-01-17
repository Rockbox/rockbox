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

PDBOX_SRC := $(call preprocess, $(PDBOXSRCDIR)/SOURCES)
PDBOX_OBJ := $(call c2obj, $(PDBOX_SRC))

# add source files to OTHERSRC to get automatic dependencies
OTHERSRC += $(PDBOX_SRC)

$(PDBOXBUILDDIR)/pdbox.rock: $(PDBOX_OBJ) $(MPEG_OBJ) $(CODECDIR)/libtlsf.a

PDBOXFLAGS = $(PLUGINFLAGS) \
             -DFIXEDPOINT -DSTATIC -DPD -DUSEAPI_ROCKBOX \
             -I$(PDBOXSRCDIR) -I$(PDBOXSRCDIR)/PDa/src \
             -I$(PDBOXSRCDIR)/TLSF-2.4.4/src

# Compile PDBox with extra flags (adapted from ZXBox)
$(PDBOXBUILDDIR)/%.o: $(PDBOXSRCDIR)/%.c $(PDBOXSRCDIR)/pdbox.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PDBOXFLAGS) -c $< -o $@
