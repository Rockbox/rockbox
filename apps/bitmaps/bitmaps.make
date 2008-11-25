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

BMPOBJ = $(BMP:$(ROOTDIR)/%.bmp=$(BUILDDIR)/%.o)

BMPHFILES = $(BMPINCDIR)/usblogo.h $(BMPINCDIR)/remote_usblogo.h \
	$(BMPINCDIR)/default_icons.h $(BMPINCDIR)/remote_default_icons.h \
	$(BMPINCDIR)/rockboxlogo.h $(BMPINCDIR)/remote_rockboxlogo.h

$(BMPHFILES): $(BMPOBJ)

# pattern rules to create .c files from .bmp, one for each subdir:
$(BUILDDIR)/apps/bitmaps/mono/%.c: $(ROOTDIR)/apps/bitmaps/mono/%.bmp $(TOOLSDIR)/bmp2rb
	$(SILENT)mkdir -p $(dir $@) $(BMPINCDIR)
	$(call PRINTS,BMP2RB $(<F))$(BMP2RB_MONO) -h $(BMPINCDIR) $< > $@

$(BUILDDIR)/apps/bitmaps/native/%.c: $(ROOTDIR)/apps/bitmaps/native/%.bmp $(TOOLSDIR)/bmp2rb
	$(SILENT)mkdir -p $(dir $@) $(BMPINCDIR)
	$(call PRINTS,BMP2RB $(<F))$(BMP2RB_NATIVE) -h $(BMPINCDIR) $< > $@

$(BUILDDIR)/apps/bitmaps/remote_mono/%.c: $(ROOTDIR)/apps/bitmaps/remote_mono/%.bmp $(TOOLSDIR)/bmp2rb
	$(SILENT)mkdir -p $(dir $@) $(BMPINCDIR)
	$(call PRINTS,BMP2RB $(<F))$(BMP2RB_REMOTEMONO) -h $(BMPINCDIR) $< > $@

$(BUILDDIR)/apps/bitmaps/remote_native/%.c: $(ROOTDIR)/apps/bitmaps/remote_native/%.bmp $(TOOLSDIR)/bmp2rb
	$(SILENT)mkdir -p $(dir $@) $(BMPINCDIR)
	$(call PRINTS,BMP2RB $(<F))$(BMP2RB_REMOTENATIVE) -h $(BMPINCDIR) $< > $@
