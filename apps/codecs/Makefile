#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES = -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I$(FIRMDIR)/common \
 -I$(FIRMDIR)/drivers -I$(APPSDIR) -Ilib
CFLAGS = $(GCCOPTS) $(INCLUDES) $(TARGET) $(EXTRA_DEFINES) \
 -DMEM=${MEMORYSIZE}

ifdef APPEXTRA
   INCLUDES += -I$(APPSDIR)/$(APPEXTRA)
endif

.PHONY: libmad liba52 libFLAC libTremor libwavpack dumb

OUTPUT = $(SOFTWARECODECS)

all: $(OUTPUT)

include $(TOOLSDIR)/make.inc


libmad:
	@echo "MAKE in libmad"
	@mkdir -p $(OBJDIR)/libmad
	@$(MAKE) -C libmad TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR)/libmad VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEMORYSIZE} OUTPUT=$(OBJDIR)/libmad.a

liba52:
	@echo "MAKE in liba52"
	@mkdir -p $(OBJDIR)/liba52
	@$(MAKE) -C liba52 TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR)/liba52 VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEMORYSIZE} OUTPUT=$(OBJDIR)/liba52.a

libFLAC:
	@echo "MAKE in libFLAC"
	@mkdir -p $(OBJDIR)/libFLAC
	@$(MAKE) -C libFLAC TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR)/libFLAC VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEMORYSIZE} OUTPUT=$(OBJDIR)/libFLAC.a

libTremor:
	@echo "MAKE in Tremor"
	@mkdir -p $(OBJDIR)/Tremor
	@$(MAKE) -C Tremor TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR)/Tremor VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEMORYSIZE} OUTPUT=$(OBJDIR)/libTremor.a

libwavpack:
	@echo "MAKE in libwavpack"
	@mkdir -p $(OBJDIR)/libwavpack
	@$(MAKE) -C libwavpack TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR)/libwavpack VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEMORYSIZE} OUTPUT=$(OBJDIR)/libwavpack.a

dumb:
	@echo "MAKE in dumb"
	@mkdir -p $(OBJDIR)/dumb
	@$(MAKE) -C dumb TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR)/dumb VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEMORYSIZE} LIBDIR=$(OBJDIR) OFLAGS="$(CFLAGS)"

clean:
	@echo "cleaning codecs"
	@rm -fr $(OBJDIR)/libmad $(OBJDIR)/liba52 $(OBJDIR)/libFLAC $(OBJDIR)/Tremor $(OBJDIR)/libwavpack
	@$(MAKE) -C libmad clean
	@$(MAKE) -C liba52 clean
	@$(MAKE) -C libFLAC clean
	@$(MAKE) -C Tremor clean
	@$(MAKE) -C libwavpack clean
