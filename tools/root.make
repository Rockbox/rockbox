#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

include $(TOOLSDIR)/functions.make

DEFINES = -DROCKBOX -DMEMORYSIZE=$(MEMORYSIZE) $(TARGET) \
	-DTARGET_ID=$(TARGET_ID) -DTARGET_NAME=\"$(MODELNAME)\" $(BUILDDATE) \
	$(EXTRA_DEFINES) # <-- -DSIMULATOR or not
INCLUDES = -I$(BUILDDIR) -I$(BUILDDIR)/lang $(TARGET_INC)

CFLAGS = $(INCLUDES) $(DEFINES) $(GCCOPTS)
PPCFLAGS = $(filter-out -g -Dmain=SDL_main,$(CFLAGS)) # cygwin sdl-config fix
ASMFLAGS = -D__ASSEMBLER__      # work around gcc 3.4.x bug with -std=gnu99, only meant for .S files
CORE_LDOPTS = $(GLOBAL_LDOPTS)  # linker ops specifically for core build

TOOLS = $(TOOLSDIR)/rdf2binary $(TOOLSDIR)/convbdf \
	$(TOOLSDIR)/codepages $(TOOLSDIR)/scramble $(TOOLSDIR)/bmp2rb \
	$(TOOLSDIR)/uclpack $(TOOLSDIR)/mkboot $(TOOLSDIR)/iaudio_bl_flash.c \
	$(TOOLSDIR)/iaudio_bl_flash.h


ifeq (,$(PREFIX))
ifdef APP_TYPE
# for sims, set simdisk/ as default
ifeq ($(APP_TYPE),sdl-sim)
RBPREFIX = simdisk
else ifeq ($(APP_TYPE),sdl-app)
RBPREFIX = /usr/local
endif

INSTALL = --install="$(RBPREFIX)"
else
# /dev/null as magic to tell it wasn't set, error out later in buildzip.pl
INSTALL = --install=/dev/null
endif
else
RBPREFIX = $(PREFIX)
INSTALL = --install="$(RBPREFIX)"
endif

RBINFO = $(BUILDDIR)/rockbox-info.txt

# list suffixes to be understood by $*
.SUFFIXES: .rock .codec .map .elf .c .S .o .bmp .a

.PHONY: all clean tags zip tools manual bin build info langs

ifeq (,$(filter clean veryclean reconf tags voice voicetools manual manual-pdf manual-html manual-zhtml manual-txt manual-ztxt manual-zip help fontzip ,$(MAKECMDGOALS)))
# none of the above
DEPFILE = $(BUILDDIR)/make.dep

endif

all: $(DEPFILE) build

# Subdir makefiles. their primary purpose is to populate SRC, OTHER_SRC,
# ASMDEFS_SRC and CORE_LIBS. But they also define special dependencies and
# compile rules
include $(TOOLSDIR)/tools.make

ifeq (,$(findstring checkwps,$(APP_TYPE)))
  ifeq (,$(findstring database,$(APP_TYPE)))
    ifeq (,$(findstring warble,$(APP_TYPE)))
      include $(FIRMDIR)/firmware.make
      include $(ROOTDIR)/apps/bitmaps/bitmaps.make
      ifeq (arch_arm,$(ARCH))
          # some targets don't use the unwarminder because they have the glibc backtrace
          ifeq (,$(filter sonynwz,$(APP_TYPE)))
            include $(ROOTDIR)/lib/unwarminder/unwarminder.make
          endif
      endif
      ifeq (,$(findstring bootloader,$(APPSDIR)))
        include $(ROOTDIR)/lib/skin_parser/skin_parser.make
        include $(ROOTDIR)/lib/tlsf/libtlsf.make
      endif
    endif
  endif
endif

#included before codecs.make and plugins.make so they see them)
ifndef APP_TYPE
  include $(ROOTDIR)/lib/libsetjmp/libsetjmp.make
  ifeq (arch_arm,$(ARCH))
    include $(ROOTDIR)/lib/arm_support/arm_support.make
  endif
endif

ifeq (,$(findstring bootloader,$(APPSDIR)))
  ifeq (,$(findstring checkwps,$(APP_TYPE)))
    include $(ROOTDIR)/lib/fixedpoint/fixedpoint.make
  endif
endif

ifneq (,$(findstring bootloader,$(APPSDIR)))
  ifneq (,$(findstring sonynwz,$(APP_TYPE)))
    include $(ROOTDIR)/firmware/target/hosted/sonynwz/sonynwz.make
  else ifneq (,$(findstring rocker,$(APP_TYPE)))
    include $(ROOTDIR)/firmware/target/hosted/agptek/rocker.make
  else ifneq (,$(findstring xduoo,$(APP_TYPE)))
    include $(ROOTDIR)/firmware/target/hosted/xduoo/xduoo.make
  else ifneq (,$(findstring erosq,$(APP_TYPE)))
    include $(ROOTDIR)/firmware/target/hosted/aigo/erosq.make
  else ifneq (,$(findstring fiio,$(APP_TYPE)))
    include $(ROOTDIR)/firmware/target/hosted/fiio/fiio.make
  else
    include $(APPSDIR)/bootloader.make
  endif
else ifneq (,$(findstring checkwps,$(APP_TYPE)))
  include $(APPSDIR)/checkwps.make
  include $(ROOTDIR)/lib/skin_parser/skin_parser.make
else ifneq (,$(findstring database,$(APP_TYPE)))
  include $(APPSDIR)/database.make
else ifneq (,$(findstring warble,$(APP_TYPE)))
  include $(ROOTDIR)/lib/rbcodec/test/warble.make
  include $(ROOTDIR)/lib/tlsf/libtlsf.make
  include $(ROOTDIR)/lib/rbcodec/rbcodec.make
else # core
  include $(APPSDIR)/lang/lang.make
  include $(APPSDIR)/apps.make
  include $(ROOTDIR)/lib/rbcodec/rbcodec.make

  ifdef ENABLEDPLUGINS
    include $(APPSDIR)/plugins/bitmaps/pluginbitmaps.make
    include $(APPSDIR)/plugins/plugins.make
  endif

  ifneq (,$(findstring sdl,$(APP_TYPE)))
    include $(ROOTDIR)/uisimulator/uisimulator.make
  endif

  ifneq (,$(findstring ypr0,$(APP_TYPE)))
    include $(ROOTDIR)/firmware/target/hosted/samsungypr/ypr0/ypr0.make
  endif

  ifneq (,$(findstring ypr1,$(APP_TYPE)))
    include $(ROOTDIR)/firmware/target/hosted/samsungypr/ypr1/ypr1.make
  endif

  ifneq (,$(findstring sonynwz,$(APP_TYPE)))
    include $(ROOTDIR)/firmware/target/hosted/sonynwz/sonynwz.make
  endif

  ifneq (,$(findstring rocker,$(APP_TYPE)))
    include $(ROOTDIR)/firmware/target/hosted/agptek/rocker.make
  endif

  ifneq (,$(findstring xduoo,$(APP_TYPE)))
    include $(ROOTDIR)/firmware/target/hosted/xduoo/xduoo.make
  endif

  ifneq (,$(findstring fiio,$(APP_TYPE)))
    include $(ROOTDIR)/firmware/target/hosted/fiio/fiio.make
  endif

  ifneq (,$(findstring erosq,$(APP_TYPE)))
    include $(ROOTDIR)/firmware/target/hosted/aigo/erosq.make
  endif

  ifneq (,$(findstring android_ndk, $(APP_TYPE)))
    include $(ROOTDIR)/firmware/target/hosted/ibasso/android_ndk.make
  else
    ifneq (,$(findstring android, $(APP_TYPE)))
	  include $(ROOTDIR)/android/android.make
    endif
  endif

  ifneq (,$(findstring pandora, $(MODELNAME)))
	include $(ROOTDIR)/packaging/pandora/pandora.make
  endif

endif # bootloader

# One or more subdir makefiles requested --gc-sections?
ifdef CORE_GCSECTIONS
  # Do not use '--gc-sections' when compiling sdl-sim
  ifneq ($(findstring sdl-sim, $(APP_TYPE)), sdl-sim)
    CORE_LDOPTS += -Wl,--gc-sections
  endif
endif # CORE_GCSECTIONS

OBJ := $(SRC:.c=.o)
OBJ := $(OBJ:.S=.o)
OBJ += $(BMP:.bmp=.o)
OBJ := $(call full_path_subst,$(ROOTDIR)/%,$(BUILDDIR)/%,$(OBJ))

build: $(TOOLS) $(BUILDDIR)/$(BINARY) $(CODECS) $(ROCKS) $(RBINFO)

$(RBINFO): $(BUILDDIR)/$(BINARY)
	$(SILENT)echo Creating $(@F)
	$(SILENT)$(TOOLSDIR)/mkinfo.pl $@

$(DEPFILE) dep:
	$(call PRINTS,Generating dependencies)
	$(call mkdepfile,$(DEPFILE)_,$(SRC))
	$(call mkdepfile,$(DEPFILE)_,$(OTHER_SRC:%.lua=))
	$(call mkdepfile,$(DEPFILE)_,$(ASMDEFS_SRC))
	$(call bmpdepfile,$(DEPFILE)_,$(BMP) $(PBMP))
	@mv $(DEPFILE)_ $(DEPFILE)

bin: $(DEPFILE) $(TOOLS) $(BUILDDIR)/$(BINARY) $(RBINFO)
rocks: $(DEPFILE) $(TOOLS) $(ROCKS)
codecs: $(DEPFILE) $(TOOLS) $(CODECS)
tools: $(TOOLS)

-include $(DEPFILE)

veryclean: clean
	$(SILENT)rm -rf $(TOOLS)

clean::
	$(SILENT)echo Cleaning build directory
	$(SILENT)rm -rf rockbox.zip rockbox.7z rockbox.tar rockbox.tar.gz \
		rockbox.tar.bz2 TAGS apps firmware tools comsim sim lang lib \
		manual *.pdf *.a credits.raw rockbox.ipod bitmaps \
		pluginbitmaps UI256.bmp rockbox-full.zip html txt \
		rockbox-manual*.zip sysfont.h rockbox-info.txt voicefontids \
		*.wav *.mp3 *.voice $(CLEANOBJS) \
		$(LINKRAM) $(LINKROM) rockbox.elf rockbox.map rockbox.bin \
		make.dep rombox.elf rombox.map rombox.bin romstart.txt \
		$(BINARY) $(FLASHFILE) uisimulator bootloader flash $(BOOTLINK) \
		rockbox.apk lang_enum.h rbversion.h

#### linking the binaries: ####

.SECONDEXPANSION:

ifeq (,$(findstring bootloader,$(APPSDIR)))
# not bootloader

OBJ += $(LANG_O)

ifndef APP_TYPE

## target build
CONFIGFILE := $(FIRMDIR)/export/config/$(MODELNAME).h
ifeq ($(wildcard $(FIRMDIR)/target/$(CPU)/$(MANUFACTURER)/app.lds),)
RAMLDS := $(FIRMDIR)/target/$(CPU)/app.lds
else
RAMLDS := $(FIRMDIR)/target/$(CPU)/$(MANUFACTURER)/app.lds
endif
LINKRAM := $(BUILDDIR)/ram.link
ROMLDS := $(FIRMDIR)/rom.lds
LINKROM := $(BUILDDIR)/rom.link

$(LINKRAM): $(RAMLDS) $(CONFIGFILE)
	$(call PRINTS,PP $(@F))
	$(call preprocess2file,$<,$@,-DLOADADDRESS=$(LOADADDRESS))

$(LINKROM): $(ROMLDS)
	$(call PRINTS,PP $(@F))
	$(call preprocess2file,$<,$@,-DLOADADDRESS=$(LOADADDRESS))

# Note: make sure -Wl,--gc-sections comes before -T in the linker options.
# Having the latter first caused crashes on (at least) mini2g.
$(BUILDDIR)/rockbox.elf : $$(OBJ) $(FIRMLIB) $(VOICESPEEXLIB) $(CORE_LIBS) $$(LINKRAM)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJ) \
		-L$(BUILDDIR)/firmware \
		-L$(RBCODEC_BLD)/codecs $(call a2lnk, $(VOICESPEEXLIB)) \
		-L$(BUILDDIR)/lib $(call a2lnk, $(CORE_LIBS)) \
		-lgcc $(CORE_LDOPTS) -T$(LINKRAM) \
		-Wl,-Map,$(BUILDDIR)/rockbox.map

$(BUILDDIR)/rombox.elf : $$(OBJ) $(FIRMLIB) $(VOICESPEEXLIB) $(CORE_LIBS) $$(LINKROM)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJ) \
		-L$(BUILDDIR)/firmware \
		-L$(RBCODEC_BLD)/codecs $(call a2lnk, $(VOICESPEEXLIB)) \
		-L$(BUILDDIR)/lib $(call a2lnk, $(CORE_LIBS)) \
		-lgcc $(CORE_LDOPTS) -T$(LINKROM) \
		-Wl,-Map,$(BUILDDIR)/rombox.map

$(BUILDDIR)/rockbox.bin : $(BUILDDIR)/rockbox.elf
	$(call PRINTS,OC $(@F))$(call objcopy,$<,$@)

$(BUILDDIR)/rombox.bin : $(BUILDDIR)/rombox.elf
	$(call PRINTS,OC $(@F))$(call objcopy,$<,$@)

$(BUILDDIR)/$(BINARY) : $(BUILDDIR)/rockbox.bin $(FLASHFILE)
	$(call PRINTS,SCRAMBLE $(notdir $@)) $(MKFIRMWARE) $< $@

$(MAXOUTFILE):
	$(call PRINTS,Creating $(@F))
	$(SILENT)echo '#include "config.h"' > $(MAXINFILE)
	$(SILENT)echo "ROM_START" >> $(MAXINFILE)
	$(call preprocess2file,$(MAXINFILE),$(MAXOUTFILE))
	$(SILENT)rm $(MAXINFILE)

# iriver
$(BUILDDIR)/rombox.iriver: $(BUILDDIR)/rombox.bin
	$(call PRINTS,Build ROM file)$(MKFIRMWARE) $< $@

endif # !APP_TYPE
endif # !bootloader


voicetools:
	$(SILENT)$(MAKE) -C $(TOOLSDIR) CC=$(HOSTCC) AR=$(HOSTAR) rbspeexenc voicefont wavtrim

tags:
	$(SILENT)rm -f TAGS
	$(SILENT)etags -o $(BUILDDIR)/TAGS $(filter-out %.o,$(SRC) $(OTHER_SRC))

fontzip:
	$(SILENT)$(TOOLSDIR)/buildzip.pl $(VERBOSEOPT) --app=$(APPLICATION) -m \"$(MODELNAME)\" -r "$(ROOTDIR)" --rbdir="$(RBDIR)" -f 1 -o rockbox-fonts.zip $(TARGET) $(BINARY)

zip: $(BUILDDIR)/rockbox.zip

ifdef NODEPS
$(BUILDDIR)/rockbox.zip:
else
$(BUILDDIR)/rockbox.zip: build
endif
	$(call PRINTS,ZIP $(notdir $@))
	$(SILENT)$(TOOLSDIR)/buildzip.pl $(VERBOSEOPT) --app=$(APPLICATION) -m \"$(MODELNAME)\" -i \"$(TARGET_ID)\"  -r "$(ROOTDIR)" --rbdir="$(RBDIR)" $(TARGET) $(BINARY)

mapzip:
	$(SILENT)find . -name "*.map" | xargs zip rockbox-maps.zip

elfzip:
	$(SILENT)find . -name "*.elf" | xargs zip rockbox-elfs.zip

fullzip:
	$(SILENT)$(TOOLSDIR)/buildzip.pl $(VERBOSEOPT) --app=$(APPLICATION) -m \"$(MODELNAME)\" -i \"$(TARGET_ID)\"  -r "$(ROOTDIR)" --rbdir="$(RBDIR)" -f 2 -o rockbox-full.zip $(TARGET) $(BINARY)

7zip:
	$(SILENT)$(TOOLSDIR)/buildzip.pl $(VERBOSEOPT) --app=$(APPLICATION) -m \"$(MODELNAME)\" -i \"$(TARGET_ID)\"  -o "rockbox.7z" -z "7za a -mx=9" -r "$(ROOTDIR)" --rbdir="$(RBDIR)" $(TARGET) $(BINARY)

tar:
	$(SILENT)rm -f rockbox.tar
	$(SILENT)$(TOOLSDIR)/buildzip.pl $(VERBOSEOPT) --app=$(APPLICATION) -m \"$(MODELNAME)\" -i \"$(TARGET_ID)\"  -o "rockbox.tar" -z "tar -cf" -r "$(ROOTDIR)" --rbdir="$(RBDIR)" $(TARGET) $(BINARY)

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

voice: voicetools $(BUILDDIR)/apps/features
	$(SILENT)for f in `cat $(BUILDDIR)/apps/features`; do feat="$$feat:$$f" ; done ; \
	for lang in `echo $(VOICELANGUAGE) |sed "s/,/ /g"`; do $(TOOLSDIR)/voice.pl -V -l=$$lang -t=$(MODELNAME)$$feat -i=$(TARGET_ID) -e="$(ENCODER)" -E="$(ENC_OPTS)" -s=$(TTS_ENGINE) -S="$(TTS_OPTS)"; done \

endif

ifeq (,$(findstring android, $(APP_TYPE)))

simext1:
	$(SILENT)mkdir -p $@

bininstall: $(BUILDDIR)/$(BINARY)
	@echo "Installing your rockbox binary in your '$(RBPREFIX)' dir"
	$(SILENT)cp $(BINARY) "$(RBPREFIX)/.rockbox/"

install: simext1
	@echo "Installing your build in your '$(RBPREFIX)' dir"
	$(SILENT)$(TOOLSDIR)/buildzip.pl $(VERBOSEOPT) --app=$(APPLICATION) -m "$(MODELNAME)" -i "$(TARGET_ID)" $(INSTALL) -z "zip -r0" -r "$(ROOTDIR)" --rbdir="$(RBDIR)" -f 0 $(TARGET) $(BINARY)

fullinstall: simext1
	@echo "Installing a full setup in your '$(RBPREFIX)' dir"
	$(SILENT)$(TOOLSDIR)/buildzip.pl $(VERBOSEOPT) --app=$(APPLICATION) -m "$(MODELNAME)" -i "$(TARGET_ID)" $(INSTALL) -z "zip -r0" -r "$(ROOTDIR)" --rbdir="$(RBDIR)" -f 2 $(TARGET) $(BINARY)

symlinkinstall: simext1
	@echo "Installing a full setup with links in your '$(RBPREFIX)' dir"
	$(SILENT)$(TOOLSDIR)/buildzip.pl $(VERBOSEOPT) --app=$(APPLICATION) -m "$(MODELNAME)" -i "$(TARGET_ID)" $(INSTALL) -z "zip -r0" -r "$(ROOTDIR)" --rbdir="$(RBDIR)" -f 2 $(TARGET) $(BINARY) -l
endif

help:
	@echo "A few helpful make targets"
	@echo ""
	@echo "all            - builds a full Rockbox (default), including tools"
	@echo "bin            - builds only the Rockbox.<target name> file"
	@echo "rocks          - builds only plugins"
	@echo "codecs         - builds only codecs"
	@echo "dep            - regenerates make dependency database"
	@echo "clean          - cleans a build directory (not tools)"
	@echo "veryclean      - cleans the build and tools directories"
	@echo "manual         - builds a manual (pdf)"
	@echo "manual-html    - HTML manual"
	@echo "manual-zip     - HTML manual (zipped)"
	@echo "manual-txt     - txt manual"
	@echo "fullzip        - creates a rockbox.zip of your build with fonts"
	@echo "zip            - creates a rockbox.zip of your build (no fonts)"
	@echo "gzip           - creates a rockbox.tar.gz of your build (no fonts)"
	@echo "bzip2          - creates a rockbox.tar.bz2 of your build (no fonts)"
	@echo "7zip           - creates a rockbox.7z of your build (no fonts)"
	@echo "fontzip        - creates rockbox-fonts.zip"
	@echo "mapzip         - creates rockbox-maps.zip with all .map files"
	@echo "elfzip         - creates rockbox-elfs.zip with all .elf files"
	@echo "pnd            - creates rockbox.pnd archive (Pandora builds only)"
	@echo "tools          - builds the tools only"
	@echo "voice          - creates the voice clips (voice builds only)"
	@echo "voicetools     - builds the voice tools only"
	@echo "install        - installs your build (at PREFIX, defaults to simdisk/ for simulators (no fonts))"
	@echo "fullinstall    - installs your build (like install, but with fonts)"
	@echo "symlinkinstall - like fullinstall, but with links instead of copying files. (Good for developing on simulator)"
	@echo "reconf         - rerun configure with the same selection"

### general compile rules:

# when source and object are in different locations (normal):
$(BUILDDIR)/%.o: $(ROOTDIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(ROOTDIR)/%.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(CFLAGS) $(ASMFLAGS) -c $< -o $@

# generated definitions for use in .S files
$(BUILDDIR)/%_asmdefs.h: $(ROOTDIR)/%_asmdefs.c
	$(call PRINTS,ASMDEFS $(@F))
	$(SILENT)mkdir -p $(dir $@)
	$(call asmdefs2file,$<,$@)

# when source and object are both in BUILDDIR (generated code):
%.o: %.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(CFLAGS) $(ASMFLAGS) -c $< -o $@

Makefile: $(TOOLSDIR)/configure
ifneq (reconf,$(MAKECMDGOALS))
	$(SILENT)echo "*** tools/configure is newer than Makefile. You should run 'make reconf'."
endif

reconf:
	$(SILENT$)$(TOOLSDIR)/configure $(CONFIGURE_OPTIONS)
