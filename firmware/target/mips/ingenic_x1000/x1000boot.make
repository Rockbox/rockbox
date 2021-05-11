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

LDSDEP := $(FIRMDIR)/export/cpu.h $(FIRMDIR)/export/config/$(MODELNAME).h

BOOTLDS := $(FIRMDIR)/target/$(CPU)/$(MANUFACTURER)/boot.lds
BOOTLINK := $(BUILDDIR)/boot.link

SPLLDS := $(FIRMDIR)/target/$(CPU)/$(MANUFACTURER)/spl.lds
SPLLINK := $(BUILDDIR)/spl.link

CLEANOBJS += $(BUILDDIR)/bootloader.* $(BUILDDIR)/spl.*

include $(FIRMDIR)/target/$(CPU)/$(MANUFACTURER)/$(MODELNAME)/boot.make

.SECONDEXPANSION:

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
