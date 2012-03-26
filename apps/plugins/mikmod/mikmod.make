#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 19082 2008-11-10 23:54:24Z zagor $
#

MIKMODSRCDIR := $(APPSDIR)/plugins/mikmod
MIKMODBUILDDIR := $(BUILDDIR)/apps/plugins/mikmod

ROCKS += $(MIKMODBUILDDIR)/mikmod.rock

MIKMOD_SRC := $(call preprocess, $(MIKMODSRCDIR)/SOURCES)
MIKMOD_OBJ := $(call c2obj, $(MIKMOD_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(MIKMOD_SRC)

MIKMODCFLAGS = $(PLUGINFLAGS) -I$(MIKMODSRCDIR) -O2

$(MIKMODBUILDDIR)/mikmod.rock: $(MIKMOD_OBJ)

# new rule needed to use extra compile flags
$(MIKMODBUILDDIR)/%.o: $(MIKMODSRCDIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(MIKMODCFLAGS) -c $< -o $@
