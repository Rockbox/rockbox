#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES= -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I. -I$(BUILDDIR)	\
 -I$(OBJDIR)

DEPFILE = $(OBJDIR)/dep-apps
LDS := $(FIRMDIR)/app.lds
ROMLDS := $(FIRMDIR)/rom.lds

ifdef ENABLEDPLUGINS
ROCKS=rocks
endif

ifdef SOFTWARECODECS
CODECS=build-codecs
endif

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

DEFINES = $(DEBUG)

DIRS = .

ifdef APPEXTRA
   DIRS += $(subst :, ,$(APPEXTRA))
   INCLUDES += $(patsubst %,-I%,$(subst :, ,$(APPEXTRA)))
endif

CFLAGS = $(GCCOPTS) $(INCLUDES) $(TARGET) $(DEFINES)			\
 -DAPPSVERSION=\"$(VERSION)\" $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE}

OBJS := $(OBJDIR)/lang.o $(SRC:%.c=$(OBJDIR)/%.o)
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
all: $(BUILDDIR)/$(BINARY) $(FLASHFILE) $(CODECS) $(ROCKS) $(ARCHOSROM)
endif

dep: $(DEPFILE)

build-codecs:
	@$(MAKE) -C codecs/lib OBJDIR=$(OBJDIR)/codecs/lib
	@$(MAKE) -C codecs OBJDIR=$(OBJDIR)/codecs

rocks:
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

$(OBJDIR)/rombox.elf : $(OBJS) $(LINKROM) $(DEPFILE) $(LIBROCKBOX)
	@echo "LD rombox.elf"
	@$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJS) -L$(BUILDDIR) -lrockbox -lgcc -L$(BUILDDIR)/firmware -T$(LINKROM) -Wl,-Map,$(OBJDIR)/rombox.map

ifndef SIMVER

$(OBJDIR)/rockbox.elf : $(OBJS) $(LINKFILE) $(DEPFILE) $(LIBROCKBOX)
	@echo "LD rockbox.elf"
	@$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJS)  -L$(BUILDDIR)/firmware -L$(BUILDDIR) -lrockbox -lgcc -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/rockbox.map

$(OBJDIR)/rockbox.bin : $(OBJDIR)/rockbox.elf
	@echo "OBJCOPY "`basename $@`
	@$(OC) -O binary $< $@

$(OBJDIR)/rombox.bin : $(OBJDIR)/rombox.elf
	@echo "OBJCOPY "`basename $@`
	@$(OC) -O binary $< $@

$(OBJDIR)/rockbox.asm: $(OBJDIR)/rockbox.bin
	$(TOOLSDIR)/sh2d -sh1 $< > $@

$(BUILDDIR)/$(BINARY) : $(OBJDIR)/rockbox.bin
	@echo "Build firmware file"
	@$(MKFIRMWARE) $< $@

else
# this is a simulator build

ifeq ($(SIMVER), win32)
# OK, this is ugly but we need it on the link line to make it do right
EXTRAOBJ = $(BUILDDIR)/sim/uisw32-res.o
LDOPTS += -lwinmm
endif


$(BUILDDIR)/$(BINARY) : $(OBJS) $(DEPFILE) $(BUILDDIR)/libsim.a \
	$(BUILDDIR)/libcomsim.a $(LIBROCKBOX)
	@echo "LD $(BINARY)"
	$(SILENT)$(CC) $(GCCOPTS) -o $@ $(OBJS) -L$(BUILDDIR) -lrockbox -lsim -lcomsim $(LDOPTS) $(EXTRAOBJ) -Wl,-Map,$(OBJDIR)/rockbox.map

endif

$(BUILDDIR)/rockbox.ucl: $(OBJDIR)/rockbox.bin
	@a=`uclpack -h 2>/dev/null`; \
	if test -n "$$a"; then \
	  echo "UCLPACK rockbox" ; \
	  uclpack --best --2e $< $@ >/dev/null 2>&1; \
	else \
	  echo "no uclpack command found, makes a fake rockbox.ucl"; \
	  echo "fake" > $@; \
	fi

$(BUILDDIR)/rombox.ucl: $(OBJDIR)/rombox.bin $(MAXOUTFILE)
	@a=`uclpack -h 2>/dev/null`; \
	if test -n "$$a"; then \
	  echo "UCLPACK rombox" ; \
	  uclpack --none $< $@ >/dev/null 2>&1; \
	  if test -s $@; then \
	    perl $(TOOLSDIR)/romsizetest.pl `cat $(MAXOUTFILE)` $<; \
	    if test $$? -ne 0; then \
	      echo "removing UCL file again, making it a fake one"; \
	      echo "fake" > $@; \
	    fi \
	  else \
	    echo "Your uclpack seems to not support --none, making a fake ucl"; \
	    echo "fake" > $@; \
	  fi \
	else \
	  echo "no uclpack command found, makes a fake rombox.ucl"; \
	  echo "fake" > $@; \
	fi

include $(TOOLSDIR)/make.inc

$(OBJDIR)/build.lang: lang/$(LANGUAGE).lang $(TOOLSDIR)/uplang
	@echo "UPLANG"
	@mkdir -p `dirname $@`
	@perl $(TOOLSDIR)/uplang lang/english.lang $< > $@

$(OBJDIR)/lang.o: $(OBJDIR)/build.lang $(TOOLSDIR)/genlang
	@echo "GENLANG"
	@mkdir -p `dirname $@`
	@perl -s $(TOOLSDIR)/genlang -p=$(BUILDDIR)/lang $<
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
	@$(MAKE) -C plugins clean OBJDIR=$(OBJDIR)/plugins
	@$(MAKE) -C codecs clean OBJDIR=$(OBJDIR)/codecs
	@rm -rf $(OBJDIR)/recorder $(OBJDIR)/player
ifdef SIMVER
	@$(MAKE) -C $(SIMDIR) clean
endif

-include $(DEPFILE)
