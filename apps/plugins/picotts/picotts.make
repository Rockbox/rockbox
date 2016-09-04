#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

PICOTTSSRCDIR := $(APPSDIR)/plugins/picotts
PICOTTSBUILDDIR := $(BUILDDIR)/apps/plugins/picotts

ROCKS += $(PICOTTSBUILDDIR)/picotts.rock

PICOTTS_SRC := $(call preprocess, $(PICOTTSSRCDIR)/SOURCES)
PICOTTS_OBJ := $(call c2obj, $(PICOTTS_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(PICOTTS_SRC)

PICOTTSCFLAGS = $(PLUGINFLAGS) -O0 -g -I$(PICOTTSSRCDIR)/libsvox

$(PICOTTSBUILDDIR)/picotts.rock: $(PICOTTS_OBJ)

# new rule needed to use extra compile flags
$(PICOTTSBUILDDIR)/%.o: $(PICOTTSSRCDIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(PICOTTSCFLAGS) -c $< -o $@
