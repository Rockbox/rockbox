#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 19082 2008-11-10 23:54:24Z zagor $
#

REVERSISRCDIR := $(APPSDIR)/plugins/reversi
REVERSIBUILDDIR := $(BUILDDIR)/apps/plugins/reversi

ROCKS += $(REVERSIBUILDDIR)/reversi.rock

REVERSI_SRC := $(call preprocess, $(REVERSISRCDIR)/SOURCES)
REVERSI_OBJ := $(call c2obj, $(REVERSI_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(REVERSI_SRC)

$(REVERSIBUILDDIR)/reversi.rock: $(REVERSI_OBJ)
# for some reason, this doesn't match the implicit rule in plugins.make,
# so we have to duplicate the link command here
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $*.elf \
		$(filter %.o, $^) \
		$(filter %.a, $^) \
		-lgcc $(PLUGINLDFLAGS)
	$(call PRINTS,LD $(@F))$(OC) -O binary $*.elf $@
