#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES += -I$(FIRMDIR)/include -I$(FIRMDIR)/export $(TARGET_INC) -I$(BUILDDIR) -I$(APPSDIR)

SIMFLAGS += $(INCLUDES) $(DEFINES) -DHAVE_CONFIG_H $(GCCOPTS)

# bootloader build is sligtly different
ifneq (,$(findstring bootloader,$(APPSDIR)))

SRC += $(call preprocess, $(APPSDIR)/SOURCES)
CLEANOBJS += $(BUILDDIR)/bootloader.*

endif #bootloader

.SECONDEXPANSION: # $$(OBJ) is not populated until after this

# bootloader build is sligtly different
ifneq (,$(findstring bootloader,$(APPSDIR)))

# We install a second font along the bootloader because sysfont is too small
# for our purpose
BL_FONT = $(ROOTDIR)/fonts/27-Adobe-Helvetica.bdf

# Limits for the bootloader sysfont: ASCII
BL_MAXCHAR = 127

$(BUILDDIR)/bootloader.fnt: $(BL_FONT) $(TOOLS)
	$(call PRINTS,CONVBDF $(subst $(ROOTDIR)/,,$<))$(TOOLSDIR)/convbdf -l $(MAXCHAR) -f -o $@ $<

$(BUILDDIR)/bootloader.elf : $$(OBJ) $(FIRMLIB) $(CORE_LIBS)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -Os -o $@ $(OBJ) \
		-L$(BUILDDIR)/firmware -lfirmware \
		-L$(BUILDDIR)/lib $(call a2lnk,$(CORE_LIBS)) \
		$(LDOPTS) $(GLOBAL_LDOPTS) -Wl,--gc-sections -Wl,-Map,$(BUILDDIR)/bootloader.map

$(BUILDDIR)/$(BINARY) : $(BUILDDIR)/bootloader.elf $(BUILDDIR)/bootloader.fnt
	$(SILENT)tar -C $(BUILDDIR) -cf $(BUILDDIR)/bootloader.tar $(notdir $^)
	$(call PRINTS,SCRAMBLE $(notdir $@))$(MKFIRMWARE) $(BUILDDIR)/bootloader.tar $@;

else # bootloader

$(BUILDDIR)/rockbox.elf : $$(OBJ) $(FIRMLIB) $(VOICESPEEXLIB) $(CORE_LIBS)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -Os -o $@ $(OBJ) \
		-L$(BUILDDIR)/firmware -lfirmware \
		-L$(RBCODEC_BLD)/codecs $(call a2lnk, $(VOICESPEEXLIB)) \
		-L$(BUILDDIR)/lib $(call a2lnk,$(CORE_LIBS)) \
		$(LDOPTS) $(GLOBAL_LDOPTS) -Wl,-Map,$(BUILDDIR)/rockbox.map

$(BUILDDIR)/rockbox.sony : $(BUILDDIR)/rockbox.elf
	$(call PRINTS,OC $(@F))$(call objcopy,$^,$@)

endif # bootloader

