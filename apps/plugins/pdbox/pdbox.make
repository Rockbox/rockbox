#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

PDBOXSRCDIR := $(APPSDIR)/plugins/pdbox
PDBOXBUILDDIR := $(BUILDDIR)/apps/plugins/pdbox

ROCKS += $(PDBOXBUILDDIR)/pdbox.rock

PDBOX_SRC := $(call preprocess, $(PDBOXSRCDIR)/SOURCES)
PDBOX_OBJ := $(call c2obj, $(PDBOX_SRC))

# add source files to OTHERSRC to get automatic dependencies
OTHER_SRC += $(PDBOX_SRC)

$(PDBOXBUILDDIR)/pdbox.rock: $(PDBOX_OBJ) $(MPEG_OBJ)

PDBOXFLAGS = $(PLUGINFLAGS) -fno-strict-aliasing
PDBOXLDFLAGS = $(PLUGINLDFLAGS) 
ifdef APP_TYPE
PDBOXLDFLAGS += -lm
endif

# Compile PDBox with extra flags (adapted from ZXBox)
$(PDBOXBUILDDIR)/%.o: $(PDBOXSRCDIR)/%.c $(PDBOXSRCDIR)/pdbox.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PDBOXFLAGS) -c $< -o $@

$(PDBOXBUILDDIR)/pdbox.rock:
	$(call PRINTS,LD $(@F))$(CC) $(PLUGINFLAGS) -o $*.elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(PDBOXLDFLAGS)
	$(SILENT)$(call objcopy,$*.elf,$@)


