#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# iPod Nano 4G: assemble the flash sequencer programs with tools/fmssasm into
# $(BUILDDIR)/fmss-nano4g.h, static arrays for nand-nano4g.c (found through
# -I$(BUILDDIR); make.dep maps the missing header to $(BUILDDIR)).

FMSS_DIR := $(FIRMDIR)/target/arm/s5l8702/ipodnano4g/fmss
FMSS_SRC := $(FMSS_DIR)/erase.s $(FMSS_DIR)/program.s $(FMSS_DIR)/read.s \
            $(FMSS_DIR)/boot_read.s
ifneq (,$(findstring -DBOOTLOADER,$(EXTRA_DEFINES)))
FMSS_SRC += $(FMSS_DIR)/boot_program.s
endif

$(BUILDDIR)/fmss-nano4g.h: $(FMSS_SRC) $(TOOLSDIR)/fmssasm
	$(call PRINTS,FMSSASM $(@F))$(TOOLSDIR)/fmssasm -s $@ $(FMSS_SRC)

CLEANOBJS += $(BUILDDIR)/fmss-nano4g.*
