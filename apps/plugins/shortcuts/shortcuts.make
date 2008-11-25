#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

SHCUTSRCDIR := $(APPSDIR)/plugins/shortcuts
SHCUTBUILDDIR := $(BUILDDIR)/apps/plugins/shortcuts

ROCKS += $(SHCUTBUILDDIR)/shortcuts_view.rock
ROCKS += $(SHCUTBUILDDIR)/shortcuts_append.rock

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(SHCUTSRCDIR)/shortcuts_common.c \
		$(SHCUTSRCDIR)/shortcuts_view.c \
		$(SHCUTSRCDIR)/shortcuts_append.c

$(SHCUTBUILDDIR)/shortcuts_view.rock: \
	$(SHCUTBUILDDIR)/shortcuts_common.o $(SHCUTBUILDDIR)/shortcuts_view.o

$(SHCUTBUILDDIR)/shortcuts_append.rock: \
	$(SHCUTBUILDDIR)/shortcuts_common.o $(SHCUTBUILDDIR)/shortcuts_append.o
