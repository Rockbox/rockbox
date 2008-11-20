#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 19082 2008-11-10 23:54:24Z zagor $
#

INCLUDES += -I$(ROOTDIR)/uisimulator/sdl -I$(ROOTDIR)/uisimulator/common \

SIMINCLUDES += -I$(ROOTDIR)/uisimulator/sdl -I$(ROOTDIR)/uisimulator/common \
	-I$(FIRMDIR)/export $(TARGET_INC) -I$(BUILDDIR) -I$(APPSDIR)

SIMFLAGS += $(SIMINCLUDES) $(DEFINES) -DHAVE_CONFIG_H $(GCCOPTS)

SIMSRC += $(call preprocess, $(ROOTDIR)/uisimulator/sdl/SOURCES)
SIMSRC += $(call preprocess, $(ROOTDIR)/uisimulator/common/SOURCES)
SIMOBJ = $(call c2obj,$(SIMSRC))
OTHER_SRC += $(SIMSRC)

SIMLIB = $(BUILDDIR)/uisimulator/libuisimulator.a
UIBMP = $(BUILDDIR)/UI256.bmp

.SECONDEXPANSION: # $$(OBJ) is not populated until after this

$(SIMLIB): $$(SIMOBJ) $(UIBMP)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rs $@ $^ >/dev/null 2>&1

$(BUILDDIR)/$(BINARY): $$(OBJ) $(SIMLIB) $(VOICESPEEXLIB) $(FIRMLIB)
	$(call PRINTS,LD $(BINARY))$(CC) -o $@ $^ $(LDOPTS) 

$(BUILDDIR)/uisimulator/%.o: $(ROOTDIR)/uisimulator/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(SIMFLAGS) -c $< -o $@

$(UIBMP): $(ROOTDIR)/uisimulator/sdl/UI-$(MODELNAME).bmp
	$(call PRINTS,CP $(@F))cp $< $@
