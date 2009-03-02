#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

CHESSBOX_SRCDIR = $(APPSDIR)/plugins/chessbox
CHESSBOX_OBJDIR = $(BUILDDIR)/apps/plugins/chessbox

CHESSBOX_SRC := $(call preprocess, $(CHESSBOX_SRCDIR)/SOURCES)
CHESSBOX_OBJ := $(call c2obj, $(CHESSBOX_SRC))

OTHER_SRC += $(CHESSBOX_SRC)

ifndef SIMVER
ifneq (,$(strip $(foreach tgt,RECORDER ONDIO,$(findstring $(tgt),$(TARGET)))))
    ### lowmem targets
    ROCKS += $(CHESSBOX_OBJDIR)/chessbox.ovl
    CHESSBOX_OUTLDS = $(CHESSBOX_OBJDIR)/chessbox.link
    CHESSBOX_OVLFLAGS = -T$(CHESSBOX_OUTLDS) -Wl,--gc-sections -Wl,-Map,$(basename $@).map
else
    ### all other targets
    ROCKS += $(CHESSBOX_OBJDIR)/chessbox.rock
endif
else
    ### simulator
    ROCKS += $(CHESSBOX_OBJDIR)/chessbox.rock
endif

ifeq ($(CPU),sh)
# sh need to retain its' -Os
CHESSBOXFLAGS = $(PLUGINFLAGS)
else
CHESSBOXFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O2
endif

$(CHESSBOX_OBJDIR)/chessbox.rock: $(CHESSBOX_OBJ)

$(CHESSBOX_OBJDIR)/chessbox.refmap: $(CHESSBOX_OBJ)

$(CHESSBOX_OUTLDS): $(PLUGIN_LDS) $(CHESSBOX_OBJDIR)/chessbox.refmap
	$(call PRINTS,PP $(@F))$(call preprocess2file,$<,$@,-DOVERLAY_OFFSET=$(shell \
		$(TOOLSDIR)/ovl_offset.pl $(CHESSBOX_OBJDIR)/chessbox.refmap))

$(CHESSBOX_OBJDIR)/chessbox.ovl: $(CHESSBOX_OBJ) $(CHESSBOX_OUTLDS)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(CHESSBOX_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(OC) -O binary $(basename $@).elf $@

# special pattern rule for compiling chessbox with extra flags
$(CHESSBOX_OBJDIR)/%.o: $(CHESSBOX_SRCDIR)/%.c $(PLUGINBITMAPLIB) $(CHESSBOX_SRCDIR)/chessbox.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(CHESSBOXFLAGS) -c $< -o $@
