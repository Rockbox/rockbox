#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

REGGEN_SRC := $(call preprocess, $(FIRMDIR)/reggen/SOURCES)

REGGEN_STAMP := $(foreach regs,$(REGGEN_SRC),$(notdir $(regs)))
REGGEN_STAMP := $(patsubst %.regs,$(BUILDDIR)/regs/%/_stamp,$(REGGEN_STAMP))

$(BUILDDIR)/regs/%/_stamp: $(FIRMDIR)/reggen/%.regs $(TOOLSDIR)/reggen
	$(SILENT)rm -rf $(@D)
	$(SILENT)mkdir -p $(@D)
	$(call PRINTS,REGGEN $(subst $(ROOTDIR)/,,$<))$(TOOLSDIR)/reggen $< -rh reggen.h -ig $(subst -,_,$*) -o $(@D)
	$(SILENT)touch $@

# 'touch' header to propagate dependency on reggen.h through to
# users of the generated headers. Might be a better way to do
# this, but this seems to be adequate.
$(BUILDDIR)/regs/%.h: $(REGGEN_STAMP) $(FIRMDIR)/export/reggen.h
	$(SILENT)touch $@

CLEANOBJS += $(BUILDDIR)/regs
