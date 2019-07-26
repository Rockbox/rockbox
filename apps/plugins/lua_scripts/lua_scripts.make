#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

LUASCR_SRCDIR := $(APPSDIR)/plugins/lua_scripts
LUASCR_BUILDDIR := $(BUILDDIR)/apps/plugins/lua_scripts
LUASCRS := $(wildcard $(LUASCR_SRCDIR)/*.lua)

#DUMMY := $(info [${LUASCRS}])

DUMMY : all

all: $(subst $(LUASCR_SRCDIR)/,$(LUASCR_BUILDDIR)/,$(LUASCRS))

$(LUASCR_BUILDDIR)/%.lua: $(LUASCR_SRCDIR)/%.lua | $(LUASCR_BUILDDIR)
	$(call PRINTS,CP $(subst $(LUASCR_SRCDIR)/,,$<))cp $< $@

$(LUASCR_BUILDDIR):
	$(call PRINTS,MKDIR $@)mkdir -p $(LUASCR_BUILDDIR)/
