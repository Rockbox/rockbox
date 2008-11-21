#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 19082 2008-11-10 23:54:24Z zagor $
#

ZXBOX_SRCDIR = $(APPSDIR)/plugins/zxbox
ZXBOX_OBJDIR = $(BUILDDIR)/apps/plugins/zxbox

ZXBOX_SRC := $(call preprocess, $(ZXBOX_SRCDIR)/SOURCES)
ZXBOX_OBJ := $(call c2obj, $(ZXBOX_SRC))

OTHER_SRC += $(ZXBOX_SRC)

ifndef SIMVER
ifneq (,$(strip $(foreach tgt,RECORDER ONDIO,$(findstring $(tgt),$(TARGET)))))
    ## archos recorder targets
    ZXBOX_INLDS := $(ZXBOX_SRCDIR)/archos.lds
    ROCKS += $(ZXBOX_OBJDIR)/zxbox.ovl
else
    ### all other targets
    ZXBOX_INLDS := $(APPSDIR)/plugins/plugin.lds
    ROCKS += $(ZXBOX_OBJDIR)/zxbox.rock
endif
else
    ### simulator
    ROCKS += $(ZXBOX_OBJDIR)/zxbox.rock
endif

ZXBOXFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O3 -funroll-loops

ifdef SIMVER
 ZXBOX_LDFLAGS = $(SHARED_FLAG) # <-- from Makefile
else
 ZXBOX_OUTLDS = $(ZXBOX_OBJDIR)/zxbox.lds
 ZXBOX_LDFLAGS = -T$(ZXBOX_OUTLDS) -Wl,--gc-sections -Wl,-Map,$(basename $@).map
endif

$(ZXBOX_OUTLDS): $(ZXBOX_INLDS) $(ZXBOX_OBJ)
	$(call PRINTS,PP $(<F))$(call preprocess2file,$<,$@)

$(ZXBOX_OBJDIR)/zxbox.rock: $(ZXBOX_OBJ) $(ZXBOX_OUTLDS) $(PLUGINBITMAPLIB)

$(ZXBOX_OBJDIR)/zxbox.ovl: $(ZXBOX_OBJ) $(ZXBOX_OUTLDS) $(PLUGINBITMAPLIB) $(PLUGINLIB)
	$(SILENT)$(CC) $(PLUGINFLAGS) -o $(basename $@).elf \
		$(filter %.o, $^) \
		$(filter %.a, $^) \
		-lgcc $(ZXBOX_LDFLAGS)
	$(call PRINTS,LD $(@F))$(OC) -O binary $(basename $@).elf $@

# special pattern rule for compiling zxbox with extra flags
$(ZXBOX_OBJDIR)/%.o: $(ZXBOX_SRCDIR)/%.c $(PLUGINBITMAPLIB) $(ZXBOX_SRCDIR)/zxbox.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(ZXBOXFLAGS) -c $< -o $@
