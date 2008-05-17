#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES= $(TARGET_INC) -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I. -I$(BUILDDIR)	\
 -I$(BUILDDIR)/bitmaps -I$(OBJDIR)

DEPFILE = $(OBJDIR)/dep-apps

LDS := $(FIRMDIR)/target/$(CPU)/$(MANUFACTURER)/app.lds

ROMLDS := $(FIRMDIR)/rom.lds

ifdef ENABLEDPLUGINS
ROCKS=rocks
endif

ifdef SOFTWARECODECS
CODECS=build-codecs

# This sets up the parameters for building and linking libspeex for core voice
VOICE=libspeex-rockbox
VOICELIB=$(BUILDDIR)/libspeex-rockbox.a
LINKVOICE= -lspeex-rockbox

endif

# Set up the bitmap libraries
BITMAPLIBS =
LINKBITMAPS =
ifneq ($(strip $(BMP2RB_MONO)),)
  BITMAPLIBS += $(BUILDDIR)/libbitmapsmono.a
  LINKBITMAPS += -lbitmapsmono
endif
ifneq ($(strip $(BMP2RB_NATIVE)),)
  BITMAPLIBS += $(BUILDDIR)/libbitmapsnative.a
  LINKBITMAPS += -lbitmapsnative
endif
ifneq ($(strip $(BMP2RB_REMOTEMONO)),)
  BITMAPLIBS += $(BUILDDIR)/libbitmapsremotemono.a
  LINKBITMAPS += -lbitmapsremotemono
endif
ifneq ($(strip $(BMP2RB_REMOTENATIVE)),)
  BITMAPLIBS += $(BUILDDIR)/libbitmapsremotenative.a
  LINKBITMAPS += -lbitmapsremotenative
endif

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

DEFINES = $(DEBUG)

DIRS = .

ifdef APPEXTRA
   DIRS += $(subst :, ,$(APPEXTRA))
   INCLUDES += $(patsubst %,-I%,$(subst :, ,$(APPEXTRA)))
endif

CFLAGS = $(INCLUDES) $(GCCOPTS) $(TARGET) $(BUILDDATE) $(DEFINES) -DTARGET_ID=$(TARGET_ID)	\
 -DAPPSVERSION=\"$(VERSION)\" $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE} \
 -DTARGET_NAME=\"$(MODELNAME)\"

OBJS2 := $(OBJDIR)/lang.o $(patsubst %.c, $(OBJDIR)/%.o, $(SRC))
OBJS = $(patsubst %.S, $(OBJDIR)/%.o, $(OBJS2))
SOURCES = $(SRC)
LINKFILE = $(OBJDIR)/linkage.lds
LINKROM = $(OBJDIR)/linkrom.lds
MAXINFILE = $(OBJDIR)/romstart.temp
MAXOUTFILE = $(OBJDIR)/romstart

LIBROCKBOX = $(BUILDDIR)/librockbox.a

ifdef SIMVER
# this is a sim build

all: $(BUILDDIR)/$(BINARY) $(CODECS) $(ROCKS)

else
# regular target build

ifdef DEBUG
REALBIN = $(OBJDIR)/rockbox.elf
else
# this is not needed to get built when doing debug builds
REALBIN = $(BUILDDIR)/$(BINARY)
endif

all: $(REALBIN) $(FLASHFILE) $(CODECS) $(ROCKS) $(ARCHOSROM)
endif

$(DEPFILE): $(BITMAPLIBS)

dep: $(DEPFILE)

features: $(OBJDIR)/features

libspeex-rockbox:
	$(SILENT)mkdir -p $(OBJDIR)/libspeex-rockbox
	$(call PRINTS,MAKE in libspeex for rockbox core)$(MAKE) -C $(APPSDIR)/codecs/libspeex ROCKBOX_VOICE_CODEC=1 OBJDIR=$(OBJDIR)/libspeex-rockbox OUTPUT=$(BUILDDIR)/libspeex-rockbox.a

build-codecs:
	$(SILENT)$(MAKE) -C codecs/lib OBJDIR=$(OBJDIR)/codecs/lib
	$(SILENT)$(MAKE) -C codecs OBJDIR=$(OBJDIR)/codecs

build-bitmapsmono:
	$(call PRINTS,MAKE in bitmaps/mono)$(MAKE) -C bitmaps/mono OBJDIR=$(OBJDIR)/bitmaps/mono

build-bitmapsnative:
	$(call PRINTS,MAKE in bitmaps/native)$(MAKE) -C bitmaps/native OBJDIR=$(OBJDIR)/bitmaps/native

build-bitmapsremotemono:
	$(call PRINTS,MAKE in bitmaps/remote_mono)$(MAKE) -C bitmaps/remote_mono OBJDIR=$(OBJDIR)/bitmaps/remote_mono

build-bitmapsremotenative:
	$(call PRINTS,MAKE in bitmaps/remote_native)$(MAKE) -C bitmaps/remote_native OBJDIR=$(OBJDIR)/bitmaps/remote_native

$(BITMAPLIBS): $(BUILDDIR)/lib%.a: build-%

rocks: build-codecs
	$(SILENT)$(MAKE) -C plugins/lib OBJDIR=$(OBJDIR)/plugins/lib
	$(SILENT)$(MAKE) -C plugins  OBJDIR=$(OBJDIR)/plugins

$(LINKFILE): $(LDS)
	$(call PRINTS,Build $(@F))cat $< | $(CC) -DLOADADDRESS=$(LOADADDRESS) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - >$@

$(LINKROM): $(ROMLDS)
	$(call PRINTS,Build $(@F))cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - >$@

$(MAXOUTFILE):
	$(SILENT)echo '#include "config.h"' > $(MAXINFILE)
	$(SILENT)echo "ROM_START" >> $(MAXINFILE)
	$(SILENT)cat $(MAXINFILE) | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - > $(MAXOUTFILE)
	$(SILENT)rm $(MAXINFILE)

$(OBJDIR)/rombox.elf : $(OBJS) $(LINKROM) $(DEPFILE) $(LIBROCKBOX) $(VOICE) $(BITMAPLIBS)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJS) -L$(BUILDDIR) -lrockbox -lgcc $(LINKVOICE) $(LINKBITMAPS) -L$(BUILDDIR)/firmware -T$(LINKROM) -Wl,-Map,$(OBJDIR)/rombox.map

ifndef SIMVER

$(OBJDIR)/rockbox.elf : $(OBJS) $(LINKFILE) $(DEPFILE) $(LIBROCKBOX) $(VOICE) $(BITMAPLIBS)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJS)  -L$(BUILDDIR)/firmware -L$(BUILDDIR) -lrockbox $(LINKVOICE) $(LINKBITMAPS) -lgcc -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/rockbox.map

$(OBJDIR)/rockbox.bin : $(OBJDIR)/rockbox.elf
	$(call PRINTS,OBJCOPY $(@F))$(OC) $(if $(filter yes, $(USE_ELF)), -S -x, -O binary) $< $@

$(OBJDIR)/rombox.bin : $(OBJDIR)/rombox.elf
	$(call PRINTS,OBJCOPY $(@F))$(OC) -O binary $< $@

$(OBJDIR)/rockbox.asm: $(OBJDIR)/rockbox.bin
	$(TOOLSDIR)/sh2d -sh1 $< > $@

#
# If there's a flashfile defined for this target (rockbox.ucl for Archos
# models) Then check if the mkfirmware script fails, as then it is (likely)
# because the image is too big and we need to create a compressed image
# instead.
#
$(BUILDDIR)/$(BINARY) : $(OBJDIR)/rockbox.bin $(FLASHFILE) 
	$(call PRINTS,Build firmware file)($(MKFIRMWARE) $< $@; \
	stat=$$?; \
	if test -n "$(FLASHFILE)"; then \
	  if test "$$stat" -ne 0; then \
	    echo "Image too big, making a compressed version!"; \
	    $(MAKE) -C $(FIRMDIR)/decompressor OBJDIR=$(BUILDDIR)/firmware/decompressor; \
	    $(MKFIRMWARE) $(BUILDDIR)/firmware/decompressor/compressed.bin $@; \
	  fi \
	fi )

else
# this is a simulator build

ifeq ($(SIMVER), win32)
# OK, this is ugly but we need it on the link line to make it do right
EXTRAOBJ = $(BUILDDIR)/sim/uisw32-res.o
LDOPTS += -lwinmm
endif

# OS X's ld does not support -Map
ifeq ($(UNAME), Darwin)
SIMULATOR_MAP=
else
#SIMULATOR_MAP=-Wl,-Map,$(OBJDIR)/rockbox.map
endif

$(BUILDDIR)/$(BINARY) : $(OBJS) $(DEPFILE) $(BUILDDIR)/libsim.a \
	$(BUILDDIR)/libcomsim.a $(LIBROCKBOX) $(VOICE) $(BITMAPLIBS)
	$(call PRINTS,LD $(BINARY))$(CC) $(GCCOPTS) -o $@ $(OBJS) -L$(BUILDDIR) -lrockbox -lsim -lcomsim $(LINKVOICE) $(LINKBITMAPS) $(LDOPTS) $(EXTRAOBJ) $(SIMULATOR_MAP)
endif

$(BUILDDIR)/rockbox.ucl: $(OBJDIR)/rockbox.bin
	$(call PRINTS,UCLPACK rockbox)$(TOOLSDIR)/uclpack --best --2e $< $@ >/dev/null 2>&1

$(BUILDDIR)/rombox.ucl: $(OBJDIR)/rombox.bin $(MAXOUTFILE)
	$(call PRINTS,UCLPACK rombox)$(TOOLSDIR)/uclpack --none $< $@ >/dev/null 2>&1; \
	perl $(TOOLSDIR)/romsizetest.pl `cat $(MAXOUTFILE)` $<; \
	if test $$? -ne 0; then \
	  echo "removing UCL file again, making it a fake one"; \
	  echo "fake" > $@; \
	fi

$(BUILDDIR)/rombox.iriver: $(OBJDIR)/rombox.bin
	$(call PRINTS,Build ROM file)$(MKFIRMWARE) $< $@

include $(TOOLSDIR)/make.inc

# apps/features.txt is a file that (is preprocessed and) lists named features
# based on defines in the config-*.h files. The named features will be passed
# to genlang and thus (translated) phrases can be used based on those names.
# button.h is included for the HAS_BUTTON_HOLD define.
#

$(OBJDIR)/features: features.txt
	$(SILENT)mkdir -p `dirname $@`
	$(SILENT)cat $< | $(HOSTCC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) \
	$(TARGET) $(DEFINES) $(EXTRA_DEFINES) -E -P -imacros "config.h" -imacros "button.h" - | \
	grep -v "^\#" | grep -v "^$$" > $@; \
	for f in `cat $(OBJDIR)/features`; do feat="$$feat:$$f" ; done ; \
	echo "$$feat" >$(OBJDIR)/genlang-features

$(OBJDIR)/lang.o: lang/$(LANGUAGE).lang $(OBJDIR)/features
	$(SILENT)for f in `cat $(OBJDIR)/features`; do feat="$$feat:$$f" ; done; \
	perl -s $(TOOLSDIR)/genlang -p=$(BUILDDIR)/lang -t=$(MODELNAME)$$feat $<
	$(call PRINTS,CC lang.c)$(CC) $(CFLAGS) -c $(BUILDDIR)/lang.c -o $@

clean:
	$(call PRINTS,cleaning apps)-rm -f $(OBJS) $(BUILDDIR)/$(BINARY) $(OBJDIR)/rockbox.asm	     \
	$(OBJDIR)/rockbox.bin $(OBJDIR)/rockbox.elf $(OBJDIR)/*.map	     \
	$(OBJDIR)/lang.o $(OBJDIR)/build.lang $(BUILDDIR)/lang.[ch]	     \
	$(LINKFILE) $(BUILDDIR)/rockbox.ucl $(LINKROM)                       \
	$(BUILDDIR)/rombox.ucl $(OBJDIR)/rombox.bin $(OBJDIR)/rombox.elf     \
	$(MAXOUTFILE) $(DEPFILE)
	$(SILENT)$(MAKE) -C bitmaps/mono clean OBJDIR=$(OBJDIR)/bitmaps/mono
	$(SILENT)$(MAKE) -C bitmaps/native clean OBJDIR=$(OBJDIR)/bitmaps/native
	$(SILENT)$(MAKE) -C bitmaps/remote_mono clean OBJDIR=$(OBJDIR)/bitmaps/remote_mono
	$(SILENT)$(MAKE) -C bitmaps/remote_native clean OBJDIR=$(OBJDIR)/bitmaps/remote_native
	$(SILENT)$(MAKE) -C plugins clean OBJDIR=$(OBJDIR)/plugins
	$(SILENT)$(MAKE) -C codecs clean OBJDIR=$(OBJDIR)/codecs
	$(SILENT)rm -rf $(OBJDIR)/recorder $(OBJDIR)/player
	$(SILENT)rm -rf $(BUILDDIR)/bitmaps
ifdef SIMVER
	$(SILENT)$(MAKE) -C $(SIMDIR) clean
endif

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPFILE)
endif
