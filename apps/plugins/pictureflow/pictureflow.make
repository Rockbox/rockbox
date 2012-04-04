#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

PICTUREFLOW_SRCDIR = $(APPSDIR)/plugins/pictureflow
PICTUREFLOW_OBJDIR = $(BUILDDIR)/apps/plugins/pictureflow

PICTUREFLOW_SRC := $(call preprocess, $(PICTUREFLOW_SRCDIR)/SOURCES)
PICTUREFLOW_OBJ := $(call c2obj, $(PICTUREFLOW_SRC))

OTHER_SRC += $(PICTUREFLOW_SRC)

ifndef APP_TYPE
ifneq (,$(strip $(foreach tgt,RECORDER ONDIO,$(findstring $(tgt),$(TARGET)))))
    ### lowmem targets
    ROCKS += $(PICTUREFLOW_OBJDIR)/pictureflow.ovl
    PICTUREFLOW_OUTLDS = $(PICTUREFLOW_OBJDIR)/picutreflow.link
    PICTUREFLOW_OVLFLAGS = -T$(PICTUREFLOW_OUTLDS) -Wl,--gc-sections -Wl,-Map,$(basename $@).map
else
    ### all other targets
    ROCKS += $(PICTUREFLOW_OBJDIR)/pictureflow.rock
endif
else
    ### simulator
    ROCKS += $(PICTUREFLOW_OBJDIR)/pictureflow.rock
endif

ifeq ($(CPU),sh)
# sh need to retain its' -Os
PICTUREFLOWFLAGS = $(PLUGINFLAGS)
else
PICTUREFLOWFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O2
endif

$(PICTUREFLOW_OBJDIR)/pictureflow.rock: $(PICTUREFLOW_OBJ)

$(PICTUREFLOW_OBJDIR)/pictureflow.refmap: $(PICTUREFLOW_OBJ)

$(PICTUREFLOW_OUTLDS): $(PLUGIN_LDS) $(PICTUREFLOW_OBJDIR)/pictureflow.refmap
	$(call PRINTS,PP $(@F))$(call preprocess2file,$<,$@,-DOVERLAY_OFFSET=$(shell \
		$(TOOLSDIR)/ovl_offset.pl $(PICTUREFLOW_OBJDIR)/pictureflow.refmap))

$(PICTUREFLOW_OBJDIR)/pictureflow.ovl: $(PICTUREFLOW_OBJ) $(PICTUREFLOW_OUTLDS)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(PICTUREFLOW_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(call objcopy,$(basename $@).elf,$@)

# special pattern rule for compiling pictureflow with extra flags
$(PICTUREFLOW_OBJDIR)/%.o: $(PICTUREFLOW_SRCDIR)/%.c $(PICTUREFLOW_SRCDIR)/pictureflow.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PICTUREFLOWFLAGS) -c $< -o $@
