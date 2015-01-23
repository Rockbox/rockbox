#             __________               __   ___
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
# Copyright (C) 2014 by Mario Basister: iBasso DX90 port
# Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
# Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.


# This is a glibc compatibility hack to provide a get_nprocs() replacement.
# The NDK ships cpu-features.c which has a compatible function android_getCpuCount()
CPUFEAT = $(ANDROID_NDK_PATH)/sources/android/cpufeatures
CPUFEAT_BUILD = $(BUILDDIR)/android-ndk/sources/android/cpufeatures
INCLUDES += -I$(CPUFEAT)
OTHER_SRC += $(CPUFEAT)/cpu-features.c
CLEANOBJS += $(CPUFEAT_BUILD)/cpu-features.o
$(CPUFEAT_BUILD)/cpu-features.o: $(CPUFEAT)/cpu-features.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(CPUFEAT)/,,$<))$(CC) -o $@ -c $(CPUFEAT)/cpu-features.c $(GCCOPTS) -Wno-unused

.SECONDEXPANSION:
.PHONY: clean dirs

DIRS += $(CPUFEAT_BUILD)

.PHONY:
$(BUILDDIR)/$(BINARY): $$(OBJ) $(FIRMLIB) $(VOICESPEEXLIB) $(CORE_LIBS) $(CPUFEAT_BUILD)/cpu-features.o
	$(call PRINTS,LD $(BINARY))$(CC) -o $@ $^ $(LDOPTS) $(GLOBAL_LDOPTS) -Wl,-Map,$(BUILDDIR)/rockbox.map
	$(call PRINTS,OC $(@F))$(call objcopy,$@,$@)

$(DIRS):
	$(SILENT)mkdir -p $@

dirs: $(DIRS)

clean::
	$(SILENT)rm -rf $(BUILDDIR)/android-ndk
