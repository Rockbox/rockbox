#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES= -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I. -I$(BUILDDIR)	\
 -I$(BUILDDIR)/bitmaps -I$(OBJDIR)

DEPFILE = $(OBJDIR)/dep-apps
LDS := $(FIRMDIR)/app.lds
ROMLDS := $(FIRMDIR)/rom.lds

ifdef ENABLEDPLUGINS
ROCKS=rocks
endif

ifdef SOFTWARECODECS
CODECS=build-codecs
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

CFLAGS = $(INCLUDES) $(GCCOPTS) $(TARGET) $(DEFINES) -DTARGET_ID=$(TARGET_ID)	\
 -DAPPSVERSION=\"$(VERSION)\" $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE}

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

build-codecs:
	@$(MAKE) -C codecs/lib OBJDIR=$(OBJDIR)/codecs/lib
	@$(MAKE) -C codecs OBJDIR=$(OBJDIR)/codecs

build-bitmaps:
	@$(MAKE) -C bitmaps OBJDIR=$(OBJDIR)/bitmaps

$(BITMAPLIBS): build-bitmaps

rocks: build-codecs
	@$(MAKE) -C plugins/lib OBJDIR=$(OBJDIR)/plugins/lib
	@$(MAKE) -C plugins  OBJDIR=$(OBJDIR)/plugins

$(LINKFILE): $(LDS)
	@echo "Build LDS file"
	@cat $< | $(CC) -DLOADADDRESS=$(LOADADDRESS) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - >$@

$(LINKROM): $(ROMLDS)
	@echo "Build LDS file for Rombox"
	@cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - >$@

$(MAXOUTFILE):
	@echo '#include "config.h"' > $(MAXINFILE)
	@echo "ROM_START" >> $(MAXINFILE)
	@cat $(MAXINFILE) | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - > $(MAXOUTFILE)
	@rm $(MAXINFILE)

$(OBJDIR)/rombox.elf : $(OBJS) $(LINKROM) $(DEPFILE) $(LIBROCKBOX) $(BITMAPLIBS)
	@echo "LD rombox.elf"
	@$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJS) -L$(BUILDDIR) -lrockbox -lgcc $(LINKBITMAPS) -L$(BUILDDIR)/firmware -T$(LINKROM) -Wl,-Map,$(OBJDIR)/rombox.map

ifndef SIMVER

$(OBJDIR)/rockbox.elf : $(OBJS) $(LINKFILE) $(DEPFILE) $(LIBROCKBOX) $(BITMAPLIBS)
	@echo "LD rockbox.elf"
	@$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJS)  -L$(BUILDDIR)/firmware -L$(BUILDDIR) -lrockbox $(LINKBITMAPS) -lgcc -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/rockbox.map

$(OBJDIR)/rockbox.bin : $(OBJDIR)/rockbox.elf
	@echo "OBJCOPY "`basename $@`
	@$(OC) -O binary $< $@

$(OBJDIR)/rombox.bin : $(OBJDIR)/rombox.elf
	@echo "OBJCOPY "`basename $@`
	@$(OC) -O binary $< $@

$(OBJDIR)/rockbox.asm: $(OBJDIR)/rockbox.bin
	$(TOOLSDIR)/sh2d -sh1 $< > $@

#
# If there's a flashfile defined for this target (rockbox.ucl for Archos
# models) Then check if the mkfirmware script fails, as then it is (likely)
# because the image is too big and we need to create a compressed image
# instead.
#
$(BUILDDIR)/$(BINARY) : $(OBJDIR)/rockbox.bin $(FLASHFILE) 
	@echo "Build firmware file"
	$(SILENT)($(MKFIRMWARE) $< $@; \
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


$(BUILDDIR)/$(BINARY) : $(OBJS) $(DEPFILE) $(BUILDDIR)/libsim.a \
	$(BUILDDIR)/libcomsim.a $(LIBROCKBOX) $(BITMAPLIBS)
	@echo "LD $(BINARY)"
	$(SILENT)$(CC) $(GCCOPTS) -o $@ $(OBJS) -L$(BUILDDIR) -lrockbox -lsim -lcomsim $(LINKBITMAPS) $(LDOPTS) $(EXTRAOBJ) -Wl,-Map,$(OBJDIR)/rockbox.map

endif

$(BUILDDIR)/rockbox.ucl: $(OBJDIR)/rockbox.bin
	$(SILENT)(echo "UCLPACK rockbox" ; \
	$(TOOLSDIR)/uclpack --best --2e $< $@ >/dev/null 2>&1;)

$(BUILDDIR)/rombox.ucl: $(OBJDIR)/rombox.bin $(MAXOUTFILE)
	$(SILENT)(echo "UCLPACK rombox" ; \
	$(TOOLSDIR)/uclpack --none $< $@ >/dev/null 2>&1; \
	perl $(TOOLSDIR)/romsizetest.pl `cat $(MAXOUTFILE)` $<; \
	if test $$? -ne 0; then \
	  echo "removing UCL file again, making it a fake one"; \
	  echo "fake" > $@; \
	fi)

include $(TOOLSDIR)/make.inc

$(OBJDIR)/lang.o: lang/$(LANGUAGE).lang
	@echo "GENLANG"
	@mkdir -p `dirname $@`
	$(SILENT)perl -s $(TOOLSDIR)/genlang -p=$(BUILDDIR)/lang -t=$(ARCHOS) $<
	@echo "CC lang.c"
	@$(CC) $(CFLAGS) -c $(BUILDDIR)/lang.c -o $@

clean:
	@echo "cleaning apps"
	@-rm -f $(OBJS) $(BUILDDIR)/$(BINARY) $(OBJDIR)/rockbox.asm	     \
	$(OBJDIR)/rockbox.bin $(OBJDIR)/rockbox.elf $(OBJDIR)/*.map	     \
	$(OBJDIR)/lang.o $(OBJDIR)/build.lang $(BUILDDIR)/lang.[ch]	     \
	$(LINKFILE) $(BUILDDIR)/rockbox.ucl $(LINKROM)                       \
	$(BUILDDIR)/rombox.ucl $(OBJDIR)/rombox.bin $(OBJDIR)/rombox.elf     \
	$(MAXOUTFILE) $(DEPFILE)
	@$(MAKE) -C bitmaps/mono clean OBJDIR=$(OBJDIR)/bitmaps/mono
	@$(MAKE) -C bitmaps/native clean OBJDIR=$(OBJDIR)/bitmaps/native
	@$(MAKE) -C bitmaps/remote_mono clean OBJDIR=$(OBJDIR)/bitmaps/remote_mono
	@$(MAKE) -C bitmaps/remote_native clean OBJDIR=$(OBJDIR)/bitmaps/remote_native
	@$(MAKE) -C plugins clean OBJDIR=$(OBJDIR)/plugins
	@$(MAKE) -C codecs clean OBJDIR=$(OBJDIR)/codecs
	@rm -rf $(OBJDIR)/recorder $(OBJDIR)/player
	@rm -rf $(BUILDDIR)/bitmaps
ifdef SIMVER
	@$(MAKE) -C $(SIMDIR) clean
endif

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPFILE)
endif
