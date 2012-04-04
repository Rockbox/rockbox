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

.SECONDEXPANSION:

$(BOOTLINK): $(BOOTLDS) $(CONFIGFILE)
	$(call PRINTS,PP $(@F))
	$(call preprocess2file,$<,$@,-DLOADADDRESS=$(LOADADDRESS))

$(BUILDDIR)/bootloader.elf: $$(OBJ) $(FIRMLIB) $(CORE_LIBS) $$(BOOTLINK)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJ) \
		-L$(BUILDDIR)/firmware -lfirmware \
		-L$(BUILDDIR)/lib $(call a2lnk, $(CORE_LIBS)) \
		-lgcc -T$(BOOTLINK) $(GLOBAL_LDOPTS) \
		-Wl,--gc-sections -Wl,-Map,$(BUILDDIR)/bootloader.map

$(BUILDDIR)/bootloader.bin : $(BUILDDIR)/bootloader.elf
	$(call PRINTS,OC $(@F))$(call objcopy,$<,$@)

$(BUILDDIR)/bootloader.asm: $(BUILDDIR)/bootloader.bin
	$(TOOLSDIR)/sh2d -sh1 $< > $@

$(BUILDDIR)/$(BINARY) : $(BUILDDIR)/bootloader.bin
	$(call PRINTS,Build bootloader file)$(MKFIRMWARE) $< $@

