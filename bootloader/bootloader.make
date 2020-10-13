#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES += -I$(APPSDIR)
SRC += $(call preprocess, $(APPSDIR)/SOURCES)

CONFIGFILE := $(FIRMDIR)/export/config/$(MODELNAME).h
BOOTLDS := $(FIRMDIR)/target/$(CPU)/$(MANUFACTURER)/boot.lds
BOOTLINK := $(BUILDDIR)/boot.link

CLEANOBJS += $(BUILDDIR)/bootloader.*

# FIXME: PP arm targets need this, otherwise we can't find __div0
EXTRA_SPECIAL_LIBS = $(call a2lnk, $(CORE_LIBS))

.SECONDEXPANSION:

$(BOOTLINK): $(BOOTLDS) $(CONFIGFILE)
	$(call PRINTS,PP $(@F))
	$(call preprocess2file,$<,$@,-DLOADADDRESS=$(LOADADDRESS))

$(BUILDDIR)/bootloader.elf: $$(OBJ) $(FIRMLIB) $(CORE_LIBS) $$(BOOTLINK)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJ) \
		-L$(BUILDDIR)/firmware -lfirmware \
		-L$(BUILDDIR)/lib $(call a2lnk, $(CORE_LIBS)) $(EXTRA_SPECIAL_LIBS) \
		-lgcc -T$(BOOTLINK) $(GLOBAL_LDOPTS) \
		-Wl,--gc-sections -Wl,-Map,$(BUILDDIR)/bootloader.map

$(BUILDDIR)/bootloader.bin : $(BUILDDIR)/bootloader.elf
	$(call PRINTS,OC $(@F))$(call objcopy,$<,$@)

$(BUILDDIR)/$(BINARY) : $(BUILDDIR)/bootloader.bin
	$(call PRINTS,Build bootloader file)$(MKFIRMWARE) $< $@
