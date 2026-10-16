#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
#

ifeq ($(PSP_FW_VERSION),)
PSP_FW_VERSION=600
endif

USE_PSPSDK_LIBC = 0

EXPAND_MEMORY = 0

STRIP    = psp-strip
MKSFO    = mksfoex
PACK_PBP = pack-pbp
FIXUP    = psp-fixup-imports
ENC      = PrxEncrypter

# CFW versions after M33 3.90 guard against expanding the
# user memory partition on PSP-1000, making MEMSIZE obsolete.
# It is now an opt-out policy with PSP_LARGE_MEMORY=0
ifeq ($(shell test $(PSP_FW_VERSION) -gt 390; echo $$?),0)
EXPAND_MEMORY = 1
ifeq ($(PSP_LARGE_MEMORY),1)
$(warning "PSP_LARGE_MEMORY" flag is not necessary targeting firmware versions above 3.90)
else ifeq ($(PSP_LARGE_MEMORY),0)
EXPAND_MEMORY = 0
endif # PSP_LARGE_MEMORY
else
ifeq ($(PSP_LARGE_MEMORY),1)
EXPAND_MEMORY = 1
endif # PSP_LARGE_MEMORY
endif # PSP_FW_VERSION

SFOFLAGS := -d MEMSIZE=$(EXPAND_MEMORY) $(SFOFLAGS)
CFLAGS += -D_PSP_FW_VERSION=$(PSP_FW_VERSION)
LDFLAGS  += -specs=$(PSPSDK)/lib/prxspecs -Wl,-q,-T$(PSPSDK)/lib/linkfile.prx

# do not forget to clean these
CLEANOBJS += $(ROOTDIR)/apps/plugins/imageviewer/plugin_crt0.o
CLEANOBJS += $(FIRMDIR)/target/hosted/game_console/psp/module/plugin_exports.o

ifeq ($(NO_FIXUP_IMPORTS), 1)
$(BINARY).elf: $(OBJS) $(EXPORT_OBJ)
	$(LINK.c) $^ $(LIBS) -o $@
else
$(BINARY).elf: $(OBJS) $(EXPORT_OBJ)
	$(LINK.c) $^ $(LIBS) -o $@
	$(FIXUP) $@
endif

$(BINARY_LIB): $(OBJS)
	$(AR) cru $@ $(OBJS)
	$(RANLIB) $@

%.prx: %.elf
	psp-prxgen $< $@

%.c: %.exp
	psp-build-exports -b $< > $@

.SECONDEXPANSION: # $$(OBJ) is not populated until after this

# main binary
$(BUILDDIR)/$(BINARY): $$(OBJ) $(FIRMLIB) $(VOICESPEEXLIB) $(CORE_LIBS)
	$(call PRINTS,LD $(BINARY))$(CC) -o $@ -Wl,--start-group $^ -Wl,--end-group $(LDOPTS) $(GLOBAL_LDOPTS) \
		-Wl,$(LDMAP_OPT),$(BUILDDIR)/rockbox.map
	psp-fixup-imports $@
	@mv $(BINARY) $(BINARY).elf
	psp-prxgen $(BINARY).elf $(BINARY).prx
	$(MAKE) -f $(ROOTDIR)/packaging/psp/psp-eboot-standalone.make

# psp custom rules to build plugins as prx modules

# codecs
$(CODECS) : $(FIRMDIR)/target/hosted/game_console/psp/module/plugin_exports.o
$(CODECDIR)/%.codec: $(CODECDIR)/%.o
	$(call PRINTS,LD $(@F))$(CC) $(CODECFLAGS) -o $(CODECDIR)/$*.elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(CODECLDFLAGS)
	psp-prxgen $(CODECDIR)/$*.elf $(CODECDIR)/$*.prx
	$(call objcopy,$(CODECDIR)/$*.prx,$@)

# plugins
$(ROCKS) : $(FIRMDIR)/target/hosted/game_console/psp/module/plugin_exports.o
$(BUILDDIR)/%.rock:
	$(call PRINTS,LD $(@F))$(CC) $(PLUGINFLAGS) -o $(BUILDDIR)/$*.elf \
		$(filter %.o, $^) \
		$(filter %.a, $+) \
		-lgcc $(PLUGINLDFLAGS)
	psp-prxgen $(BUILDDIR)/$*.elf $(BUILDDIR)/$*.prx
	$(call objcopy_plugin,$(BUILDDIR)/$*.prx,$@)

# imgviewer
IMGVBUILDDIR := $(BUILDDIR)/apps/plugins/imageviewer
IMGDEC_OVLFLAGS = $(PLUGINLDFLAGS) -Wl,$(LDMAP_OPT),$(IMGVBUILDDIR)/$*.map

$(IMGVBUILDDIR)/%.ovl: $(IMGDEC_OUTLDS)
	$(call PRINTS,LD $(@F))$(CC) $(IMGDECFLAGS) -o $(IMGVBUILDDIR)/$*.elf \
		$(filter-out $(PLUGIN_CRT0),$(filter %.o, $^)) \
		$(filter %.a, $+) \
		-lgcc $(IMGDEC_OVLFLAGS)
	psp-prxgen $(IMGVBUILDDIR)/$*.elf $(IMGVBUILDDIR)/$*.prx
	$(SILENT)$(call objcopy_plugin,$(IMGVBUILDDIR)/$*.prx,$@)


