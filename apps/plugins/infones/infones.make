#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: $
#

INFONESSRCDIR := $(APPSDIR)/plugins/infones
INFONESBUILDDIR := $(BUILDDIR)/apps/plugins/infones

ROCKS += $(INFONESBUILDDIR)/infones.rock

INFONES_SRC := $(call preprocess, $(INFONESSRCDIR)/SOURCES)
INFONES_OBJ := $(call c2obj, $(INFONES_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(INFONES_SRC)

INFONESCFLAGS += $(PLUGINFLAGS) -O3 -w

# new rule needed to use extra compile flags
$(INFONESBUILDDIR)/%.o: $(INFONESSRCDIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(INFONESCFLAGS) -c $< -o $@

$(INFONESBUILDDIR)/infones.rock: $(INFONES_OBJ)
	$(call PRINTS,LD $(@F))
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $*.elf \
		$(filter %.o, $^) \
		$(filter %.a, $^) \
		-lgcc $(PLUGINLDFLAGS) -Wl,-Map,$(basename $@).map
ifdef SIMVER
	$(SILENT)cp $*.elf $@
else
	$(SILENT)$(OC) -O binary $*.elf $@
endif
