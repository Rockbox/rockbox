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
    ## archos recorder targets
    CHESSBOX_INLDS := $(CHESSBOX_SRCDIR)/archos.lds
    ROCKS += $(CHESSBOX_OBJDIR)/chessbox.ovl
else
    ### all other targets
    CHESSBOX_INLDS := $(APPSDIR)/plugins/plugin.lds
    ROCKS += $(CHESSBOX_OBJDIR)/chessbox.rock
endif
    CHESSBOX_OVLFLAGS = -T$(CHESSBOX_OUTLDS) -Wl,--gc-sections -Wl,-Map,$(basename $@).map
    CHESSBOX_OUTLDS = $(CHESSBOX_OBJDIR)/chessbox.lds
else
    ### simulator
    ROCKS += $(CHESSBOX_OBJDIR)/chessbox.rock
    CHESSBOX_OVLFLAGS = $(SHARED_FLAG) # <-- from Makefile
endif

ifeq ($(CPU),sh)
# sh need to retain its' -Os
CHESSBOXFLAGS = $(PLUGINFLAGS)
else
CHESSBOXFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O2
endif

$(CHESSBOX_OUTLDS): $(CHESSBOX_INLDS) $(CHESSBOX_OBJ)
	$(call PRINTS,PP $(<F))$(call preprocess2file,$<,$@)

$(CHESSBOX_OBJDIR)/chessbox.rock: $(CHESSBOX_OBJ) $(CHESSBOX_OUTLDS) $(PLUGINBITMAPLIB)

$(CHESSBOX_OBJDIR)/chessbox.ovl: $(CHESSBOX_OBJ) $(CHESSBOX_OUTLDS) $(PLUGINBITMAPLIB)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $^) \
		-lgcc $(CHESSBOX_OVLFLAGS)
	$(call PRINTS,LD $(@F))$(OC) -O binary $(basename $@).elf $@

# special pattern rule for compiling chessbox with extra flags
$(CHESSBOX_OBJDIR)/%.o: $(CHESSBOX_SRCDIR)/%.c $(PLUGINBITMAPLIB) $(CHESSBOX_SRCDIR)/chessbox.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(CHESSBOXFLAGS) -c $< -o $@
