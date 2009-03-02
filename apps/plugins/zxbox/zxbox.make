#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

ZXBOX_SRCDIR = $(APPSDIR)/plugins/zxbox
ZXBOX_OBJDIR = $(BUILDDIR)/apps/plugins/zxbox

ZXBOX_SRC := $(call preprocess, $(ZXBOX_SRCDIR)/SOURCES)
ZXBOX_OBJ := $(call c2obj, $(ZXBOX_SRC))

OTHER_SRC += $(ZXBOX_SRC)

ifndef SIMVER
ifneq (,$(strip $(foreach tgt,RECORDER ONDIO,$(findstring $(tgt),$(TARGET)))))
    ## lowmem targets
    ROCKS += $(ZXBOX_OBJDIR)/zxbox.ovl
    ZXBOX_OUTLDS = $(ZXBOX_OBJDIR)/zxbox.link
    ZXBOX_LDFLAGS = -T$(ZXBOX_OUTLDS) -Wl,--gc-sections -Wl,-Map,$(basename $@).map
else
    ### all other targets
    ROCKS += $(ZXBOX_OBJDIR)/zxbox.rock
endif
else
    ### simulator
    ROCKS += $(ZXBOX_OBJDIR)/zxbox.rock
endif

ZXBOXFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O3 -funroll-loops

$(ZXBOX_OBJDIR)/zxbox.rock: $(ZXBOX_OBJ)

$(ZXBOX_OBJDIR)/zxbox.refmap: $(ZXBOX_OBJ)

$(ZXBOX_OUTLDS): $(PLUGIN_LDS) $(ZXBOX_OBJDIR)/zxbox.refmap
	$(call PRINTS,PP $(@F))$(call preprocess2file,$<,$@,-DOVERLAY_OFFSET=$(shell \
		$(TOOLSDIR)/ovl_offset.pl $(ZXBOX_OBJDIR)/zxbox.refmap))

$(ZXBOX_OBJDIR)/zxbox.ovl: $(ZXBOX_OBJ) $(ZXBOX_OUTLDS)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(ZXBOX_LDFLAGS)
	$(call PRINTS,LD $(@F))$(OC) -O binary $(basename $@).elf $@

# special pattern rule for compiling zxbox with extra flags
$(ZXBOX_OBJDIR)/%.o: $(ZXBOX_SRCDIR)/%.c $(PLUGINBITMAPLIB) $(ZXBOX_SRCDIR)/zxbox.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(ZXBOXFLAGS) -c $< -o $@
