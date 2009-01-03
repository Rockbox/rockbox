#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

ROCKBOY_SRCDIR = $(APPSDIR)/plugins/rockboy
ROCKBOY_OBJDIR = $(BUILDDIR)/apps/plugins/rockboy

ROCKBOY_SRC := $(call preprocess, $(ROCKBOY_SRCDIR)/SOURCES)
ROCKBOY_SRC += $(ROOTDIR)/firmware/common/sscanf.c
ROCKBOY_OBJ := $(call c2obj, $(ROCKBOY_SRC))

OTHER_SRC += $(ROCKBOY_SRC)

ifndef SIMVER
ifneq (,$(findstring RECORDER,$(TARGET)))
    ## archos recorder targets
    ROCKBOY_INLDS := $(ROCKBOY_SRCDIR)/archos.lds
    ROCKS += $(ROCKBOY_OBJDIR)/rockboy.ovl
else
    ### all other targets
    ROCKBOY_INLDS := $(APPSDIR)/plugins/plugin.lds
    ROCKS += $(ROCKBOY_OBJDIR)/rockboy.rock
endif
    ROCKBOY_OVLFLAGS = -T$(ROCKBOY_OUTLDS) -Wl,--gc-sections -Wl,-Map,$(basename $@).map
    ROCKBOY_OUTLDS = $(ROCKBOY_OBJDIR)/rockboy.lds
else
    ### simulator
    ROCKS += $(ROCKBOY_OBJDIR)/rockboy.rock
    ROCKBOY_OVLFLAGS = $(SHARED_FLAG) # <-- from Makefile
endif

$(ROCKBOY_OUTLDS): $(ROCKBOY_INLDS) $(ROCKBOY_OBJ)
	$(call PRINTS,PP $(<F))$(call preprocess2file,$<,$@)

$(ROCKBOY_OBJDIR)/rockboy.rock: $(ROCKBOY_OBJ) $(ROCKBOY_OUTLDS) $(PLUGINBITMAPLIB)

$(ROCKBOY_OBJDIR)/rockboy.ovl: $(ROCKBOY_OBJ) $(ROCKBOY_OUTLDS) $(PLUGINBITMAPLIB)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $^) \
		-lgcc $(ROCKBOY_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(OC) -O binary $(basename $@).elf $@
