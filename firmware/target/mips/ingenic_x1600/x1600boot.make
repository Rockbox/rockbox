#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# X1600 bootloader, flash SPL and USB recovery stage.
# mkspl-x1600 supplies the BootROM header CRCs. The returning USB stage
# uses a separate link address from the flash SPL.

include $(ROOTDIR)/lib/microtar/microtar.make

INCLUDES += -I$(APPSDIR)
SRC += $(call preprocess, $(APPSDIR)/SOURCES)

LDSDEP := $(FIRMDIR)/export/x1600.h $(FIRMDIR)/export/config.h

BOOTLDS := $(FIRMDIR)/target/$(CPU)/$(MANUFACTURER)/boot.lds
BOOTLINK := $(BUILDDIR)/boot.link
BOOTEXT := $(suffix $(BINARY))

SPLLDS := $(FIRMDIR)/target/$(CPU)/$(MANUFACTURER)/spl.lds
SPLLINK := $(BUILDDIR)/spl.link
SPLBINARY := spl$(BOOTEXT)

STAGE1LDS := $(FIRMDIR)/target/$(CPU)/$(MANUFACTURER)/usbstage1.lds
STAGE1LINK := $(BUILDDIR)/usbstage1.link

BLINFO = $(BUILDDIR)/bootloader-info.txt

CLEANOBJS += $(BUILDDIR)/bootloader.* $(BUILDDIR)/spl.* $(BUILDDIR)/usbstage1.*

.SECONDEXPANSION:

### Bootloader

$(BOOTLINK): $(BOOTLDS) $(LDSDEP)
	$(call PRINTS,PP $(@F))
	$(call preprocess2file,$<,$@,)

$(BUILDDIR)/bootloader.elf: $$(OBJ) $(FIRMLIB) $(CORE_LIBS) $$(BOOTLINK)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJ) \
		-L$(BUILDDIR)/firmware -lfirmware \
		-L$(BUILDDIR)/lib $(call a2lnk, $(CORE_LIBS)) \
		-lgcc -T$(BOOTLINK) $(GLOBAL_LDOPTS) \
		-Wl,--gc-sections -Wl,-Map,$(BUILDDIR)/bootloader.map

$(BUILDDIR)/bootloader.bin: $(BUILDDIR)/bootloader.elf
	$(call PRINTS,OC $(@F))$(call objcopy,$<,$@)

$(BUILDDIR)/bootloader.ucl: $(BUILDDIR)/bootloader.bin
	$(call PRINTS,UCLPACK $(@F))$(TOOLSDIR)/uclpack --nrv2e -9 $< $@ >/dev/null


### SPL (flash boot, linked at 0x80001800)

$(SPLLINK): $(SPLLDS) $(LDSDEP)
	$(call PRINTS,PP $(@F))
	$(call preprocess2file,$<,$@,)

$(BUILDDIR)/spl.elf: $$(OBJ) $(FIRMLIB) $(CORE_LIBS) $$(SPLLINK)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJ) \
		-L$(BUILDDIR)/firmware -lfirmware \
		-L$(BUILDDIR)/lib $(call a2lnk, $(CORE_LIBS)) \
		-lgcc -T$(SPLLINK) $(GLOBAL_LDOPTS) \
		-Wl,--gc-sections -Wl,-Map,$(BUILDDIR)/spl.map

$(BUILDDIR)/spl.bin: $(BUILDDIR)/spl.elf
	$(call PRINTS,OC $(@F))$(call objcopy,$<,$@)

$(BUILDDIR)/$(SPLBINARY): $(BUILDDIR)/spl.bin
	$(call PRINTS,MKSPL $(@F))$(MKFIRMWARE) $< $@


### USB stage1 (BootROM recovery, linked at 0x8000a000)

$(STAGE1LINK): $(STAGE1LDS) $(LDSDEP)
	$(call PRINTS,PP $(@F))
	$(call preprocess2file,$<,$@,)

$(BUILDDIR)/usbstage1.elf: $$(OBJ) $(FIRMLIB) $(CORE_LIBS) $$(STAGE1LINK)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJ) \
		-L$(BUILDDIR)/firmware -lfirmware \
		-L$(BUILDDIR)/lib $(call a2lnk, $(CORE_LIBS)) \
		-lgcc -T$(STAGE1LINK) $(GLOBAL_LDOPTS) \
		-Wl,--gc-sections -Wl,-Map,$(BUILDDIR)/usbstage1.map

# A plain objcopy -O binary would try to pad the file from offset 0x8000a000.
$(BUILDDIR)/usbstage1.bin: $(BUILDDIR)/usbstage1.elf
	$(call PRINTS,OC $(@F))$(OC) -O binary -j .text \
		--change-section-lma .text=0 $< $@

.PHONY: usbstage1
usbstage1: $(BUILDDIR)/usbstage1.bin


### Generating the update package

# suppress regenerating bootloader-info if nothing has changed
BLVERSION:=$(SVNVERSION)
OLDBLVERSION:=$(shell head -n1 $(BLINFO) 2>/dev/null || echo "NOREVISION")

ifneq ($(BLVERSION),$(OLDBLVERSION))
.PHONY: $(BLINFO)
endif

$(BLINFO):
	$(call PRINTS,GEN $(@F))echo $(SVNVERSION) > $@

# Package the flash SPL and raw USB stage separately. bootloader2.ucl keeps
# older jztool versions from using the X1000 load address for this image.

$(BUILDDIR)/usbstage1$(suffix $(BINARY)): $(BUILDDIR)/usbstage1.bin
	$(call PRINTS,CP $(@F))cp $< $@

$(BUILDDIR)/$(BINARY): $(BUILDDIR)/$(SPLBINARY) \
					   $(BUILDDIR)/usbstage1$(suffix $(BINARY)) \
					   $(BUILDDIR)/bootloader2.ucl \
					   $(BLINFO)
	$(call PRINTS,TAR $(@F))tar -C $(BUILDDIR) \
		--numeric-owner --no-acls --no-xattrs --no-selinux \
		--mode=0644 --owner=0 --group=0 \
		-cf $@ $(call full_path_subst,$(BUILDDIR)/%,%,$^)

$(BUILDDIR)/bootloader2.ucl: $(BUILDDIR)/bootloader.ucl
	$(call PRINTS,CP $(@F))cp $< $@
