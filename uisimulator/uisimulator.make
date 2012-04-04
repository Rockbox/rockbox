#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES += -I$(ROOTDIR)/uisimulator/bitmaps -I$(ROOTDIR)/uisimulator/common -I$(ROOTDIR)/uisimulator/buttonmap $\
	-I$(FIRMDIR)/include -I$(FIRMDIR)/export $(TARGET_INC) -I$(BUILDDIR) -I$(APPSDIR)

SIMFLAGS += $(INCLUDES) $(DEFINES) -DHAVE_CONFIG_H $(GCCOPTS)

SIMSRC += $(call preprocess, $(ROOTDIR)/uisimulator/common/SOURCES)
SIMSRC += $(call preprocess, $(ROOTDIR)/uisimulator/buttonmap/SOURCES)
SIMOBJ = $(call c2obj,$(SIMSRC))
OTHER_SRC += $(SIMSRC)

SIMLIB = $(BUILDDIR)/uisimulator/libuisimulator.a
ifeq (yes,$(APPLICATION))
UIBMP=
else
UIBMP=$(BUILDDIR)/UI256.bmp
endif

.SECONDEXPANSION: # $$(OBJ) is not populated until after this

$(SIMLIB): $$(SIMOBJ) $(UIBMP)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

$(BUILDDIR)/$(BINARY): $$(OBJ) $(FIRMLIB) $(VOICESPEEXLIB) $(CORE_LIBS) $(SIMLIB)
	$(call PRINTS,LD $(BINARY))$(CC) -o $@ $^ $(SIMLIB) $(LDOPTS) $(GLOBAL_LDOPTS) \
	-Wl,-Map,$(BUILDDIR)/rockbox.map
	$(SILENT)$(call objcopy,$@,$@)

$(BUILDDIR)/uisimulator/%.o: $(ROOTDIR)/uisimulator/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(SIMFLAGS) -c $< -o $@

$(UIBMP): $(ROOTDIR)/uisimulator/bitmaps/UI-$(MODELNAME).bmp
	$(call PRINTS,CP $(@F))cp $< $@
