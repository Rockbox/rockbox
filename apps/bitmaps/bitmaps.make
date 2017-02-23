#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

BITMAPDIR = $(ROOTDIR)/apps/bitmaps
BMPINCDIR = $(BUILDDIR)/bitmaps

INCLUDES += -I$(BMPINCDIR)

ifneq ($(strip $(BMP2RB_MONO)),)
BMP = $(call preprocess, $(BITMAPDIR)/mono/SOURCES)
endif
ifneq ($(strip $(BMP2RB_NATIVE)),)
BMP += $(call preprocess, $(BITMAPDIR)/native/SOURCES)
endif
ifneq ($(strip $(BMP2RB_REMOTEMONO)),)
BMP += $(call preprocess, $(BITMAPDIR)/remote_mono/SOURCES)
endif
ifneq ($(strip $(BMP2RB_REMOTENATIVE)),)
BMP += $(call preprocess, $(BITMAPDIR)/remote_native/SOURCES)
endif

BMPOBJ = $(call full_path_subst,$(ROOTDIR)/%.bmp,$(BUILDDIR)/%.o,$(BMP))

BMPHFILES = $(BMPINCDIR)/usblogo.h $(BMPINCDIR)/remote_usblogo.h \
	$(BMPINCDIR)/default_icons.h $(BMPINCDIR)/remote_default_icons.h \
	$(BMPINCDIR)/rockboxlogo.h $(BMPINCDIR)/remote_rockboxlogo.h \
	$(BMPINCDIR)/rockboxicon.h $(BMPINCDIR)/toolsicon.h

$(BMPHFILES): $(BMPOBJ)

# pattern rules to create .c files from .bmp, one for each subdir:
$(BUILDDIR)/apps/bitmaps/mono/%.c: $(ROOTDIR)/apps/bitmaps/mono/%.bmp $(TOOLSDIR)/bmp2rb
	$(SILENT)mkdir -p $(dir $@) $(BMPINCDIR)
	$(call PRINTS,BMP2RB $(<F))$(BMP2RB_MONO) -b -h $(BMPINCDIR) $< > $@

$(BUILDDIR)/apps/bitmaps/native/%.c: $(ROOTDIR)/apps/bitmaps/native/%.bmp $(TOOLSDIR)/bmp2rb
	$(SILENT)mkdir -p $(dir $@) $(BMPINCDIR)
	$(call PRINTS,BMP2RB $(<F))$(BMP2RB_NATIVE) -b -h $(BMPINCDIR) $< > $@

$(BUILDDIR)/apps/bitmaps/remote_mono/%.c: $(ROOTDIR)/apps/bitmaps/remote_mono/%.bmp $(TOOLSDIR)/bmp2rb
	$(SILENT)mkdir -p $(dir $@) $(BMPINCDIR)
	$(call PRINTS,BMP2RB $(<F))$(BMP2RB_REMOTEMONO) -b -h $(BMPINCDIR) $< > $@

$(BUILDDIR)/apps/bitmaps/remote_native/%.c: $(ROOTDIR)/apps/bitmaps/remote_native/%.bmp $(TOOLSDIR)/bmp2rb
	$(SILENT)mkdir -p $(dir $@) $(BMPINCDIR)
	$(call PRINTS,BMP2RB $(<F))$(BMP2RB_REMOTENATIVE) -b -h $(BMPINCDIR) $< > $@
