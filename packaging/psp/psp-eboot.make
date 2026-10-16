#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# todo: move these to configure script
STRIP    = psp-strip
MKSFO    = mksfoex
PACK_PBP = pack-pbp
FIXUP    = psp-fixup-imports

PSPSDK := $$(shell psp-config --pspsdk-path)

INCLUDES += -I$(APPSDIR)
SRC += $(call preprocess, $(APPSDIR)/SOURCES)

CONFIGFILE := $(FIRMDIR)/export/config/$(MODELNAME).h

CLEANOBJS += $(BUILDDIR)/bootloader.* $(BUILDDIR)/PARAM.SFO

EXPORT_OBJ=$(PSPSDK)/lib/prxexports.o

SFOFLAGS := -d MEMSIZE=0 $(SFOFLAGS)

# EBOOT parameters
PSP_EBOOT_TITLE = rockbox
PSP_EBOOT_SFO 	= $(BUILDDIR)/PARAM.SFO
PSP_EBOOT_ICON 	= $(ROOTDIR)/packaging/psp/ICON0.png
PSP_EBOOT_ICON1 = NULL
PSP_EBOOT_PIC0 	= NULL
PSP_EBOOT_PIC1 	= $(ROOTDIR)/packaging/psp/PIC1.png
PSP_EBOOT_SND0 	= $(ROOTDIR)/packaging/psp/SND0.at3
PSP_EBOOT_PSAR 	= NULL

.SECONDEXPANSION:

$(PSP_EBOOT_SFO):
	$(MKSFO) $(SFOFLAGS) '$(PSP_EBOOT_TITLE)' $@

$(BUILDDIR)/bootloader.elf: $$(OBJ) $(EXPORT_OBJ)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -o $@ $(OBJ) $(LDOPTS) $(GLOBAL_LDOPTS) \
		-Wl,-Map,$(BUILDDIR)/bootloader.map
	$(FIXUP) $@

$(BUILDDIR)/$(BINARY): $(BUILDDIR)/bootloader.elf $(PSP_EBOOT_SFO)
	$(STRIP) $(BUILDDIR)/bootloader.elf -o $(BUILDDIR)/bootloader_strip.elf
	$(PACK_PBP) $(BUILDDIR)/$(BINARY) $(PSP_EBOOT_SFO) $(PSP_EBOOT_ICON)  \
	$(PSP_EBOOT_ICON1) $(PSP_EBOOT_PIC0) $(PSP_EBOOT_PIC1)  \
	$(PSP_EBOOT_SND0) $(BUILDDIR)/bootloader_strip.elf $(PSP_EBOOT_PSAR)
	-rm -f $(BUILDDIR)/bootloader_strip.elf