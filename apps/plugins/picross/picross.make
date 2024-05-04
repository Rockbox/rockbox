#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

PICRSCR_SRCDIR := $(APPSDIR)/plugins/picross
PICRSCR_BUILDDIR := $(BUILDDIR)/apps/plugins/.picross
PICRSCRS := $(wildcard $(PICRSCR_SRCDIR)/*.picross)

#DUMMY := $(info [${PICRSCRS}])

DUMMY : all

all: $(subst $(PICRSCR_SRCDIR)/,$(PICRSCR_BUILDDIR)/,$(PICRSCRS))

$(PICRSCR_BUILDDIR)/%.picross: $(PICRSCR_SRCDIR)/%.picross | $(PICRSCR_BUILDDIR)
	$(call PRINTS,CP $(subst $(APPSDIR)/,,$<))cp $< $@

$(PICRSCR_BUILDDIR):
	$(call PRINTS,MKDIR $@)mkdir -p $(PICRSCR_BUILDDIR)/
