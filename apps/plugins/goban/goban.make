#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#


GOBAN_SRCDIR := $(APPSDIR)/plugins/goban
GOBAN_BUILDDIR := $(BUILDDIR)/apps/plugins/goban

GOBAN_SRC := $(call preprocess, $(GOBAN_SRCDIR)/SOURCES)
GOBAN_OBJ := $(call c2obj, $(GOBAN_SRC))

OTHER_SRC += $(GOBAN_SRC)

ifndef SIMVER
ifneq (,$(strip $(foreach tgt,RECORDER ONDIO,$(findstring $(tgt),$(TARGET)))))
    ### archos recorder targets
    GOBAN_INLDS := $(GOBAN_SRCDIR)/archos.lds
    ROCKS += $(GOBAN_BUILDDIR)/goban.ovl
else
    ### all other targets
    GOBAN_INLDS := $(APPSDIR)/plugins/plugin.lds
    ROCKS += $(GOBAN_BUILDDIR)/goban.rock
endif
    GOBAN_OVLFLAGS = -T$(GOBAN_OUTLDS) -Wl,--gc-sections -Wl,-Map,$(basename $@).map
    GOBAN_OUTLDS = $(GOBAN_BUILDDIR)/goban.lds
else
    ### simulator
    ROCKS += $(GOBAN_BUILDDIR)/goban.rock
    GOBAN_OVLFLAGS = $(SHARED_FLAG) # <-- from Makefile
endif

$(GOBAN_OUTLDS): $(GOBAN_INLDS) $(GOBAN_OBJ)
	$(call PRINTS,PP $(<F))$(call preprocess2file,$<,$@)

$(GOBAN_BUILDDIR)/goban.rock: $(GOBAN_OBJ) $(GOBAN_OUTLDS)

$(GOBAN_BUILDDIR)/goban.ovl: $(GOBAN_OBJ) $(GOBAN_OUTLDS)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $^) \
		-lgcc $(GOBAN_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(OC) -O binary $(basename $@).elf $@

