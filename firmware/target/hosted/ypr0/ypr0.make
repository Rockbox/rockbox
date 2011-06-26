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

.SECONDEXPANSION: # $$(OBJ) is not populated until after this

$(BUILDDIR)/rockbox.elf : $$(OBJ) $(FIRMLIB) $(VOICESPEEXLIB) $(CORE_LIBS)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -Os -o $@ $(OBJ) \
		-L$(BUILDDIR)/firmware -lfirmware \
		-L$(RBCODEC_BLD)/codecs $(call a2lnk, $(VOICESPEEXLIB)) \
		-L$(BUILDDIR)/lib $(call a2lnk,$(CORE_LIBS)) \
		$(LDOPTS) $(GLOBAL_LDOPTS) -Wl,-Map,$(BUILDDIR)/rockbox.map

$(BUILDDIR)/rockbox : $(BUILDDIR)/rockbox.elf
	$(call PRINTS,OC $(@F))$(call objcopy,$^,$@)
