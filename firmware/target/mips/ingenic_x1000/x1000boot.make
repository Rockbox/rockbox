#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

include $(ROOTDIR)/lib/microtar/microtar.make

INCLUDES += -I$(APPSDIR)
SRC += $(call preprocess, $(APPSDIR)/SOURCES)

LDSDEP := $(FIRMDIR)/export/cpu.h $(FIRMDIR)/export/config/$(MODELNAME).h

BOOTLDS := $(FIRMDIR)/target/$(CPU)/$(MANUFACTURER)/boot.lds
BOOTLINK := $(BUILDDIR)/boot.link
BOOTEXT := $(suffix $(BINARY))

SPLLDS := $(FIRMDIR)/target/$(CPU)/$(MANUFACTURER)/spl.lds
SPLLINK := $(BUILDDIR)/spl.link
SPLBINARY := spl$(BOOTEXT)

BLINFO = $(BUILDDIR)/bootloader-info.txt

CLEANOBJS += $(BUILDDIR)/bootloader.* $(BUILDDIR)/spl.*

# Currently not needed
#include $(FIRMDIR)/target/$(CPU)/$(MANUFACTURER)/$(MODELNAME)/boot.make

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


### SPL

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


### Generating the update package

# suppress regenerating bootloader-info if nothing has changed
BLVERSION:=$(SVNVERSION)
OLDBLVERSION:=$(shell head -n1 $(BLINFO) 2>/dev/null || echo "NOREVISION")

ifneq ($(BLVERSION),$(OLDBLVERSION))
.PHONY: $(BLINFO)
endif

$(BLINFO):
	$(call PRINTS,GEN $(@F))echo $(SVNVERSION) > $@

# The "binary" is actually an update package which is just a tar archive
$(BUILDDIR)/$(BINARY): $(BUILDDIR)/$(SPLBINARY) \
					   $(BUILDDIR)/bootloader.ucl \
					   $(BLINFO)
	$(call PRINTS,TAR $(@F))tar -C $(BUILDDIR) \
		--numeric-owner --no-acls --no-xattrs --no-selinux \
		--mode=0644 --owner=0 --group=0 \
		-cf $@ $(call full_path_subst,$(BUILDDIR)/%,%,$^)
