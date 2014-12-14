#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

Z80ESRCDIR := $(APPSDIR)/plugins/z80e
Z80EBUILDDIR := $(BUILDDIR)/apps/plugins/z80e

ROCKS += $(Z80EBUILDDIR)/z80e.rock

Z80E_SRC := $(call preprocess, $(Z80ESRCDIR)/SOURCES)
Z80E_OBJ := $(call c2obj, $(Z80E_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(Z80E_SRC)

Z80EFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O2 -I$(Z80ESRCDIR)/libz80e/include/z80e -I$(Z80ESRCDIR) -I$(Z80ESRCDIR)/libz80e/include

$(Z80EBUILDDIR)/z80e.rock: $(Z80E_OBJ) $(TLSFLIB)

$(Z80EBUILDDIR)/%.o: $(Z80ESRCDIR)/%.c $(Z80ESRCDIR)/z80e.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(Z80EFLAGS) -c $< -o $@
