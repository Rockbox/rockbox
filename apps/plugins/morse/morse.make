#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

MORSESRCDIR := $(APPSDIR)/plugins/morse
MORSEBUILDDIR := $(BUILDDIR)/apps/plugins/morse

ROCKS += $(MORSEBUILDDIR)/morse.rock

MORSE_SRC := $(call preprocess, $(MORSESRCDIR)/SOURCES)
MORSE_OBJ := $(call c2obj, $(MORSE_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(MORSE_SRC)

MORSEFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O2

$(MORSEBUILDDIR)/morse.rock: $(MORSE_OBJ) $(TLSFLIB)

$(MORSEBUILDDIR)/%.o: $(MORSESRCDIR)/%.c $(MORSESRCDIR)/morse.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(MORSEFLAGS) -c $< -o $@
