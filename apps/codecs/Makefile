#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES = -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I$(FIRMDIR)/common \
 -I$(FIRMDIR)/drivers -I$(APPSDIR) -Ilib -I$(BUILDDIR)
CFLAGS = $(GCCOPTS) $(INCLUDES) $(TARGET) $(EXTRA_DEFINES) \
 -DMEM=${MEMORYSIZE}

ifdef APPEXTRA
   INCLUDES += -I$(APPSDIR)/$(APPEXTRA)
endif

.PHONY: libmad liba52 libFLAC libTremor libwavpack dumb libmusepack

OUTPUT = $(SOFTWARECODECS)

all: $(OUTPUT)

include $(TOOLSDIR)/make.inc


libmad:
	@echo "MAKE in libmad"
	@mkdir -p $(OBJDIR)/libmad
	@$(MAKE) -C libmad OBJDIR=$(OBJDIR)/libmad OUTPUT=$(BUILDDIR)/libmad.a

liba52:
	@echo "MAKE in liba52"
	@mkdir -p $(OBJDIR)/liba52
	@$(MAKE) -C liba52 OBJDIR=$(OBJDIR)/liba52 OUTPUT=$(BUILDDIR)/liba52.a

libFLAC:
	@echo "MAKE in libFLAC"
	@mkdir -p $(OBJDIR)/libFLAC
	@$(MAKE) -C libFLAC OBJDIR=$(OBJDIR)/libFLAC OUTPUT=$(BUILDDIR)/libFLAC.a

libTremor:
	@echo "MAKE in Tremor"
	@mkdir -p $(OBJDIR)/Tremor
	@$(MAKE) -C Tremor OBJDIR=$(OBJDIR)/Tremor OUTPUT=$(BUILDDIR)/libTremor.a

libwavpack:
	@echo "MAKE in libwavpack"
	@mkdir -p $(OBJDIR)/libwavpack
	@$(MAKE) -C libwavpack OBJDIR=$(OBJDIR)/libwavpack OUTPUT=$(BUILDDIR)/libwavpack.a

dumb:
	@echo "MAKE in dumb"
	@mkdir -p $(OBJDIR)/dumb
	@$(MAKE) -C dumb OBJDIR=$(OBJDIR)/dumb LIBDIR=$(BUILDDIR) OFLAGS="$(CFLAGS)"

libmusepack:
	@echo "MAKE in libmusepack"
	@mkdir -p $(OBJDIR)/libmusepack
	@$(MAKE) -C libmusepack OBJDIR=$(OBJDIR)/libmusepack OUTPUT=$(BUILDDIR)/libmusepack.a

clean:
	@echo "cleaning codecs"
	$(SILENT)rm -fr $(OBJDIR)/libmad $(BUILDDIR)/libmad.a $(OBJDIR)/liba52 $(OBJDIR)/libFLAC $(OBJDIR)/Tremor $(OBJDIR)/libwavpack $(OBJDIR)/dumb $(BUILDDIR)/libdumb.a $(BUILDDIR)/libdumbd.a $(OBJDIR)/libmusepack $(BUILDDIR)/libmusepack.a
	@$(MAKE) -C libmad clean OBJDIR=$(OBJDIR)/libmad
	@$(MAKE) -C liba52 clean OBJDIR=$(OBJDIR)/liba52
	@$(MAKE) -C libFLAC clean OBJDIR=$(OBJDIR)/libFLAC
	@$(MAKE) -C Tremor clean OBJDIR=$(OBJDIR)/Tremor
	@$(MAKE) -C libwavpack clean OBJDIR=$(OBJDIR)/libwavpack
	@$(MAKE) -C libmusepack clean OBJDIR=$(OBJDIR)/libmusepack
	@$(MAKE) -C dumb clean OBJDIR=$(OBJDIR)/dumb
