#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES= -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I. -I$(OBJDIR)

DEPFILE = $(OBJDIR)/dep-apps
LDS := $(FIRMDIR)/app.lds
ROMLDS := $(FIRMDIR)/rom.lds

ifdef ENABLEDPLUGINS
ROCKS=rocks
endif

ifdef SOFTWARECODECS
CODECS=build-codecs
endif

SRC := $(shell cat SOURCES | gcc -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) \
$(TARGET) $(DEFINES) $(EXTRA_DEFINES) -E -P -include "config.h" - )
DIRS = .

ifdef APPEXTRA
   DIRS += $(APPEXTRA)
   INCLUDES += -I$(APPEXTRA)
endif

CFLAGS = $(GCCOPTS) $(INCLUDES) $(TARGET) $(DEFINES)			\
 -DAPPSVERSION=\"$(VERSION)\" $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE}

OBJS := $(OBJDIR)/lang.o $(SRC:%.c=$(OBJDIR)/%.o)
SOURCES = $(SRC)
LINKFILE = $(OBJDIR)/linkage.lds
LINKROM = $(OBJDIR)/linkrom.lds
MAXINFILE = $(OBJDIR)/romstart.temp
MAXOUTFILE = $(OBJDIR)/romstart

ifdef SIMVER
# this is a sim build
COMMONLIB = $(ROOTDIR)/uisimulator/common/libcomsim.a

all: $(COMMONLIB) $(OBJDIR)/$(BINARY) $(CODECS) $(ROCKS)
	@$(MAKE) -C $(SIMDIR)

$(COMMONLIB):
	@$(MAKE) -C $(ROOTDIR)/uisimulator/common
else
# regular target build
all: $(OBJDIR)/$(BINARY) $(FLASHFILE) $(CODECS) $(ROCKS) $(ARCHOSROM)
endif

dep: $(DEPFILE)

build-codecs:
	@$(MAKE) -C codecs

rocks:
	@$(MAKE) -C plugins/lib
	@$(MAKE) -C plugins

$(OBJDIR)/librockbox.a:
	@$(MAKE) -C $(FIRMDIR)

$(LINKFILE): $(LDS)
	@echo "Build LDS file"
	@cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - >$@

$(LINKROM): $(ROMLDS)
	@echo "Build LDS file for Rombox"
	@cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - >$@

$(MAXOUTFILE):
	@echo '#include "config.h"' > $(MAXINFILE)
	@echo "ROM_START" >> $(MAXINFILE)
	@cat $(MAXINFILE) | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - > $(MAXOUTFILE)
	@rm $(MAXINFILE)

$(OBJDIR)/rombox.elf : $(OBJS) $(LINKROM) $(OBJDIR)/librockbox.a $(DEPFILE)
	@echo "LD rombox.elf"
	@$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJS) -L$(OBJDIR) -lrockbox -lgcc -L$(FIRMDIR) -T$(LINKROM) -Wl,-Map,$(OBJDIR)/rombox.map

ifndef SIMVER

$(OBJDIR)/rockbox.elf : $(OBJS) $(LINKFILE) $(OBJDIR)/librockbox.a $(DEPFILE)
	@echo "LD rockbox.elf"
	$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJS) -L$(OBJDIR) -lrockbox -lgcc -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/rockbox.map

$(OBJDIR)/rockbox.bin : $(OBJDIR)/rockbox.elf
	@echo "OBJCOPY $<"
	@$(OC) -O binary $< $@

$(OBJDIR)/rombox.bin : $(OBJDIR)/rombox.elf
	@echo "OBJCOPY $<"
	@$(OC) -O binary $< $@

$(OBJDIR)/rockbox.asm: $(OBJDIR)/rockbox.bin
	$(TOOLSDIR)/sh2d -sh1 $< > $@

$(OBJDIR)/$(BINARY) : $(OBJDIR)/rockbox.bin
	@echo "Build firmware file"
	@$(MKFIRMWARE) $< $@

else
# this is a simulator build

ifeq ($(SIMVER), win32)
# OK, this is ugly but we need it on the link line to make it do right
EXTRAOBJ = $(OBJDIR)/uisw32-res.o
endif


$(OBJDIR)/$(BINARY) : $(OBJS) $(OBJDIR)/librockbox.a $(DEPFILE) $(OBJDIR)/libsim.a $(OBJDIR)/libcomsim.a
	@echo "LD $(BINARY)"
	$(CC) $(GCCOPTS) -o $@ $(OBJS) -L$(OBJDIR) -lrockbox -lsim -lcomsim $(LDOPTS) $(EXTRAOBJ) -Wl,-Map,$(OBJDIR)/rockbox.map

$(OBJDIR)/libsim.a:
	@$(MAKE) -C $(SIMDIR)

$(OBJDIR)/libcomsim.a:
	@$(MAKE) -C $(ROOTDIR)/uisimulator/common
endif

$(OBJDIR)/rockbox.ucl: $(OBJDIR)/rockbox.bin
	@a=`uclpack -h 2>/dev/null`; \
	if test -n "$$a"; then \
	  echo "UCLPACK rockbox" ; \
	  uclpack --best --2e $< $@ >/dev/null 2>&1; \
	else \
	  echo "no uclpack command found, makes a fake rockbox.ucl"; \
	  echo "fake" > $@; \
	fi

$(OBJDIR)/rombox.ucl: $(OBJDIR)/rombox.bin $(MAXOUTFILE)
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

$(OBJDIR)/credits.raw: $(DOCSDIR)/CREDITS
	perl credits.pl < $< > $@

$(OBJDIR)/credits.o: credits.c credits.h $(OBJDIR)/credits.raw
	@mkdir -p `dirname $@`
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

include $(TOOLSDIR)/make.inc

$(OBJDIR)/build.lang: lang/$(LANGUAGE).lang $(TOOLSDIR)/uplang
	@echo "UPLANG"
	@perl $(TOOLSDIR)/uplang lang/english.lang $< > $@

$(OBJDIR)/lang.o: $(OBJDIR)/build.lang $(TOOLSDIR)/genlang
	@echo "GENLANG"
	@perl -s $(TOOLSDIR)/genlang -p=$(OBJDIR)/lang $<
	@echo "CC lang.c"
	@$(CC) $(CFLAGS) -c $(OBJDIR)/lang.c -o $@

clean:
	@echo "cleaning apps"
	@-rm -f $(OBJS) $(OBJDIR)/$(BINARY) $(OBJDIR)/rockbox.asm	\
	$(OBJDIR)/rockbox.bin $(OBJDIR)/rockbox.elf $(OBJDIR)/*.map	\
	$(OBJDIR)/lang.o $(OBJDIR)/build.lang $(OBJDIR)/lang.[ch]	\
	$(OBJDIR)/credits.raw $(LINKFILE) $(OBJDIR)/rockbox.ucl		\
	$(LINKROM) $(OBJDIR)/rombox.ucl $(OBJDIR)/rombox.bin		\
	$(OBJDIR)/rombox.elf $(MAXOUTFILE) $(DEPFILE)
	@$(MAKE) -C plugins clean
	@$(MAKE) -C codecs clean
	@rm -rf $(OBJDIR)/recorder $(OBJDIR)/player
ifdef SIMVER
	@$(MAKE) -C $(SIMDIR) clean
endif

-include $(DEPFILE)
