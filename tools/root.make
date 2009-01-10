#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

include $(TOOLSDIR)/functions.make

DEFINES = -DROCKBOX -DMEMORYSIZE=$(MEMORYSIZE) -DMEM=$(MEMORYSIZE) $(TARGET) \
	-DTARGET_ID=$(TARGET_ID) -DTARGET_NAME=\"$(MODELNAME)\" \
	-DAPPSVERSION=\"$(VERSION)\" $(BUILDDATE) \
	$(EXTRA_DEFINES) # <-- -DSIMULATOR or not
INCLUDES = -I$(BUILDDIR) $(TARGET_INC)

CFLAGS = $(INCLUDES) $(DEFINES) $(GCCOPTS) 
PPCFLAGS = $(filter-out -Dmain=SDL_main,$(CFLAGS)) # cygwin sdl-config fix

TOOLS = $(TOOLSDIR)/rdf2binary $(TOOLSDIR)/convbdf \
	$(TOOLSDIR)/codepages $(TOOLSDIR)/scramble $(TOOLSDIR)/bmp2rb \
	$(TOOLSDIR)/uclpack $(TOOLSDIR)/mktccboot $(TOOLSDIR)/mkboot

RBINFO = $(BUILDDIR)/rockbox-info.txt

# list suffixes to be understood by $*
.SUFFIXES: .rock .codec .map .elf .c .S .o .bmp .a

.PHONY: all clean tags zip tools manual bin build info langs

ifeq (,$(filter clean veryclean tags voice voicetools manual manual-pdf manual-html manual-zhtml manual-txt manual-ztxt manual-zip help fontzip ,$(MAKECMDGOALS)))
# none of the above
DEPFILE = $(BUILDDIR)/make.dep

endif

all: $(DEPFILE) build

# Subdir makefiles. their primary purpose is to populate SRC & OTHER_SRC
# but they also define special dependencies and compile rules
include $(TOOLSDIR)/tools.make
include $(FIRMDIR)/firmware.make
include $(ROOTDIR)/apps/bitmaps/bitmaps.make

ifneq (,$(findstring bootloader,$(APPSDIR)))
  include $(APPSDIR)/bootloader.make
else ifneq (,$(findstring bootbox,$(APPSDIR)))
  BOOTBOXLDOPTS = -Wl,--gc-sections
  include $(APPSDIR)/bootbox.make
else
  include $(APPSDIR)/apps.make
  include $(APPSDIR)/lang/lang.make

  ifdef SOFTWARECODECS
    include $(APPSDIR)/codecs/codecs.make
  endif

  ifdef ENABLEDPLUGINS
    include $(APPSDIR)/plugins/bitmaps/pluginbitmaps.make
    include $(APPSDIR)/plugins/plugins.make
  endif

  ifdef SIMVER
    include $(ROOTDIR)/uisimulator/uisimulator.make
  endif
endif # bootloader

OBJ := $(SRC:.c=.o)
OBJ := $(OBJ:.S=.o)
OBJ += $(BMP:.bmp=.o)
OBJ := $(subst $(ROOTDIR),$(BUILDDIR),$(OBJ))

build: $(TOOLS) $(BUILDDIR)/$(BINARY) $(CODECS) $(ROCKS) $(ARCHOSROM) $(RBINFO)

$(RBINFO): $(BUILDDIR)/$(BINARY)
	$(SILENT)echo Creating $(@F)
	$(SILENT)$(TOOLSDIR)/mkinfo.pl $@

$(DEPFILE) dep:
	$(call PRINTS,Generating dependencies)
	@echo foo > /dev/null # there must be a "real" command in the rule
	$(call mkdepfile,$(DEPFILE),$(SRC) $(OTHER_SRC))
	$(call bmpdepfile,$(DEPFILE),$(BMP) $(PBMP))

bin: $(DEPFILE) $(TOOLS) $(BUILDDIR)/$(BINARY)
rocks: $(DEPFILE) $(TOOLS) $(ROCKS)
codecs: $(DEPFILE) $(TOOLS) $(CODECS)

-include $(DEPFILE)

veryclean: clean
	$(SILENT)rm -rf $(TOOLS)

clean:
	$(SILENT)echo Cleaning build directory
	$(SILENT)rm -rf rockbox.zip rockbox.7z rockbox.tar rockbox.tar.gz \
		rockbox.tar.bz2 TAGS apps firmware comsim sim lang.[ch] \
		manual *.pdf *.a credits.raw rockbox.ipod bitmaps \
		pluginbitmaps UI256.bmp rockbox-full.zip html txt \
		rockbox-manual*.zip sysfont.h rockbox-info.txt voicefontids \
		*.wav *.mp3 *.voice max_language_size.h $(CLEANOBJS) \
		$(LINKRAM) $(LINKROM) rockbox.elf rockbox.map rockbox.bin \
		make.dep rombox.elf rombox.map rombox.bin rombox.ucl romstart.txt \
		$(BINARY) $(FLASHFILE) uisimulator bootloader flash $(BOOTLINK)

#### linking the binaries: ####

.SECONDEXPANSION:

ifeq (,$(findstring bootloader,$(APPSDIR)))
# not bootloader

OBJ += $(LANG_O)

ifndef SIMVER

## target build
RAMLDS := $(FIRMDIR)/target/$(CPU)/$(MANUFACTURER)/app.lds
LINKRAM := $(BUILDDIR)/ram.link
ROMLDS := $(FIRMDIR)/rom.lds
LINKROM := $(BUILDDIR)/rom.link


$(LINKRAM): $(RAMLDS)
	$(call PRINTS,PP $(@F))
	$(call preprocess2file,$<,$@,-DLOADADDRESS=$(LOADADDRESS))

$(LINKROM): $(ROMLDS)
	$(call PRINTS,PP $(@F))
	$(call preprocess2file,$<,$@,-DLOADADDRESS=$(LOADADDRESS))

$(BUILDDIR)/rockbox.elf : $$(OBJ) $$(FIRMLIB) $$(VOICESPEEXLIB) $$(LINKRAM)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJ) \
		-L$(BUILDDIR)/firmware -lfirmware \
		-L$(BUILDDIR)/apps/codecs $(VOICESPEEXLIB:lib%.a=-l%) \
		-lgcc $(BOOTBOXLDOPTS) \
		-T$(LINKRAM) -Wl,-Map,$(BUILDDIR)/rockbox.map

$(BUILDDIR)/rombox.elf : $$(OBJ) $$(FIRMLIB) $$(VOICESPEEXLIB) $$(LINKROM)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJ) \
		$(VOICESPEEXLIB) $(FIRMLIB) -lgcc -L$(BUILDDIR)/firmware \
		-T$(LINKROM) -Wl,-Map,$(BUILDDIR)/rombox.map

$(BUILDDIR)/rockbox.bin : $(BUILDDIR)/rockbox.elf
	$(call PRINTS,OC $(@F))$(OC) $(if $(filter yes, $(USE_ELF)), -S -x, -O binary) $< $@

$(BUILDDIR)/rombox.bin : $(BUILDDIR)/rombox.elf
	$(call PRINTS,OC $(@F))$(OC) -O binary $< $@

#
# If there's a flashfile defined for this target (rockbox.ucl for Archos
# models) Then check if the mkfirmware script fails, as then it is (likely)
# because the image is too big and we need to create a compressed image
# instead.
#
$(BUILDDIR)/$(BINARY) : $(BUILDDIR)/rockbox.bin $(FLASHFILE)
	$(call PRINTS,SCRAMBLE $(notdir $@))($(MKFIRMWARE) $< $@; \
	stat=$$?; \
	if test -n "$(FLASHFILE)"; then \
	  if test "$$stat" -ne 0; then \
	    echo "Image too big, making a compressed version!"; \
	    $(MAKE) -C $(FIRMDIR)/decompressor OBJDIR=$(BUILDDIR)/firmware/decompressor; \
	    $(MKFIRMWARE) $(BUILDDIR)/firmware/decompressor/compressed.bin $@; \
	  fi \
	fi )

# archos
$(BUILDDIR)/rockbox.ucl: $(BUILDDIR)/rockbox.bin
	$(call PRINTS,UCLPACK $(@F))$(TOOLSDIR)/uclpack --best --2e -b1048576 $< $@ >/dev/null 2>&1

MAXINFILE = $(BUILDDIR)/temp.txt
MAXOUTFILE = $(BUILDDIR)/romstart.txt

$(BUILDDIR)/rombox.ucl: $(BUILDDIR)/rombox.bin $(MAXOUTFILE)
	$(call PRINTS,UCLPACK $(@F))$(TOOLSDIR)/uclpack --none $< $@ >/dev/null; \
		perl $(TOOLSDIR)/romsizetest.pl `cat $(MAXOUTFILE)` $<; \
		if test $$? -ne 0; then \
		  echo "removing UCL file again, making it a fake one"; \
		  echo "fake" > $@; \
		fi

$(MAXOUTFILE):
	$(call PRINTS,Creating $(@F))
	$(SILENT)$(shell echo '#include "config.h"' > $(MAXINFILE))
	$(SILENT)$(shell echo "ROM_START" >> $(MAXINFILE))
	$(call preprocess2file,$(MAXINFILE),$(MAXOUTFILE))
	$(SILENT)rm $(MAXINFILE)

# iriver
$(BUILDDIR)/rombox.iriver: $(BUILDDIR)/rombox.bin
	$(call PRINTS,Build ROM file)$(MKFIRMWARE) $< $@

endif # !SIMVER
endif # !bootloader


voicetools:
	$(SILENT)$(MAKE) -C $(TOOLSDIR) CC=$(HOSTCC) AR=$(HOSTAR) rbspeexenc voicefont wavtrim

tags:
	$(SILENT)rm -f TAGS
	$(SILENT)etags -o $(BUILDDIR)/TAGS $(filter-out %.o,$(SRC) $(OTHER_SRC))

fontzip:
	$(SILENT)$(TOOLSDIR)/buildzip.pl $(VERBOSEOPT) -t \"$(MODELNAME)\" -r "$(ROOTDIR)" --rbdir="$(RBDIR)" -f 1 -o rockbox-fonts.zip $(TARGET) $(BINARY)

zip:
	$(SILENT)for f in `cat $(BUILDDIR)/apps/features`; do feat="$$feat:$$f" ; done ; \
	$(TOOLSDIR)/buildzip.pl $(VERBOSEOPT) -t \"$(MODELNAME)$$feat\" -i \"$(TARGET_ID)\"  -r "$(ROOTDIR)" --rbdir="$(RBDIR)" $(TARGET) $(BINARY)

mapzip:
	$(SILENT)find . -name "*.map" | xargs zip rockbox-maps.zip

fullzip:
	$(SILENT)for f in `cat $(BUILDDIR)/apps/features`; do feat="$$feat:$$f" ; done; \
	$(TOOLSDIR)/buildzip.pl $(VERBOSEOPT) -t \"$(MODELNAME)$$feat\" -i \"$(TARGET_ID)\"  -r "$(ROOTDIR)" --rbdir="$(RBDIR)" -f 2 -o rockbox-full.zip $(TARGET) $(BINARY)

7zip:
	$(SILENT)for f in `cat $(BUILDDIR)/apps/features`; do feat="$$feat:$$f" ; done; \
	$(TOOLSDIR)/buildzip.pl $(VERBOSEOPT) -t \"$(MODELNAME)$$feat\" -i \"$(TARGET_ID)\"  -o "rockbox.7z" -z "7za a -mx=9" -r "$(ROOTDIR)" --rbdir="$(RBDIR)" $(TARGET) $(BINARY)

tar:
	$(SILENT)rm -f rockbox.tar
	$(SILENT)for f in `cat $(BUILDDIR)/apps/features`; do feat="$$feat:$$f" ; done; \
	$(TOOLSDIR)/buildzip.pl $(VERBOSEOPT) -t \"$(MODELNAME)$$feat\" -i \"$(TARGET_ID)\"  -o "rockbox.tar" -z "tar -cf" -r "$(ROOTDIR)" --rbdir="$(RBDIR)" $(TARGET) $(BINARY)

bzip2: tar
	$(SILENT)bzip2 -f9 rockbox.tar

gzip: tar
	$(SILENT)gzip -f9 rockbox.tar

manual manual-pdf:
	$(SILENT)$(MAKE) -C $(MANUALDIR) OBJDIR=$(BUILDDIR)/manual manual-pdf
manual-html:
	$(SILENT)$(MAKE) -C $(MANUALDIR) OBJDIR=$(BUILDDIR)/manual manual-html
manual-zhtml: manual-zip
manual-txt:
	$(SILENT)$(MAKE) -C $(MANUALDIR) OBJDIR=$(BUILDDIR)/manual manual-txt
manual-ztxt:
	$(SILENT)$(MAKE) -C $(MANUALDIR) OBJDIR=$(BUILDDIR)/manual manual-txt-zip
manual-zip:
	$(SILENT)$(MAKE) -C $(MANUALDIR) OBJDIR=$(BUILDDIR)/manual manual-zip

ifdef TTS_ENGINE

voice: voicetools features
	$(SILENT)for f in `cat $(BUILDDIR)/apps/features`; do feat="$$feat:$$f" ; done ; \
	for lang in `echo $(VOICELANGUAGE) |sed "s/,/ /g"`; do $(TOOLSDIR)/voice.pl -V -l=$$lang -t=$(MODELNAME)$$feat -i=$(TARGET_ID) -e="$(ENCODER)" -E="$(ENC_OPTS)" -s=$(TTS_ENGINE) -S="$(TTS_OPTS)"; done \

endif

ifdef SIMVER

install:
	@echo "Installing your build in your 'simdisk' dir"
	$(SILENT)for f in `cat $(BUILDDIR)/apps/features`; do feat="$$feat:$$f" ; done; \
	$(TOOLSDIR)/buildzip.pl $(VERBOSEOPT) -t \"$(MODELNAME)$$feat\" -i \"$(TARGET_ID)\" -s -r "$(ROOTDIR)" --rbdir="$(RBDIR)" -f 0 $(TARGET) $(BINARY)

fullinstall:
	@echo "Installing a full setup in your 'simdisk' dir"
	$(SILENT)for f in `cat $(BUILDDIR)/apps/features`; do feat="$$feat:$$f" ; done; \
	$(TOOLSDIR)/buildzip.pl $(VERBOSEOPT) -t \"$(MODELNAME)$$feat\" -i \"$(TARGET_ID)\" -s -r "$(ROOTDIR)" --rbdir="$(RBDIR)" -f 2 $(TARGET) $(BINARY)

endif

help:
	@echo "A few helpful make targets"
	@echo ""
	@echo "all         - builds a full Rockbox (default), including tools"
	@echo "bin         - builds only the Rockbox.<target name> file"
	@echo "rocks       - builds only plugins"
	@echo "codecs      - builds only codecs"
	@echo "dep         - regenerates make dependency database"
	@echo "clean       - cleans a build directory (not tools)"
	@echo "veryclean   - cleans the build and tools directories"
	@echo "manual      - builds a manual (pdf)"
	@echo "manual-html - HTML manual"
	@echo "manual-zip  - HTML manual (zipped)"
	@echo "manual-txt  - txt manual"
	@echo "fullzip     - creates a rockbox.zip of your build with fonts"
	@echo "zip         - creates a rockbox.zip of your build (no fonts)"
	@echo "gzip        - creates a rockbox.tar.gz of your build (no fonts)"
	@echo "bzip2       - creates a rockbox.tar.bz2 of your build (no fonts)"
	@echo "7zip        - creates a rockbox.7z of your build (no fonts)"
	@echo "fontzip     - creates rockbox-fonts.zip"
	@echo "mapzip      - creates rockbox-maps.zip with all .map files"
	@echo "tools       - builds the tools only"
	@echo "voice       - creates the voice clips (voice builds only)"
	@echo "voicetools  - builds the voice tools only"
	@echo "install     - installs your build (for simulator builds only, no fonts)"
	@echo "fullinstall - installs your build (for simulator builds only, with fonts)"
	@echo "reconf      - rerun configure with the same selection"

### general compile rules:

# when source and object are in different locations (normal):
$(BUILDDIR)/%.o: $(ROOTDIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(ROOTDIR)/%.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(CFLAGS) -c $< -o $@

# when source and object are both in BUILDDIR (generated code):
%.o: %.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(CFLAGS) -c $< -o $@

Makefile: $(TOOLSDIR)/configure
	$(SILENT)echo "*** tools/configure is newer than Makefile. You should run 'make reconf'."

reconf:
	$(SILENT)$(TOOLSDIR)/configure $(CONFIGURE_OPTIONS)
