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

OUTPUT = $(OBJDIR)/libmad.a $(OBJDIR)/liba52.a $(OBJDIR)/libFLAC.a $(OBJDIR)/libTremor.a

all: $(OUTPUT)

include $(TOOLSDIR)/make.inc

$(OBJDIR)/libmad.a:
	@echo "MAKE in libmad"
	@mkdir -p $(OBJDIR)/libmad
	@$(MAKE) -C libmad TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR) VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEMORYSIZE}

$(OBJDIR)/liba52.a:
	@echo "MAKE in liba52"
	@mkdir -p $(OBJDIR)/liba52
	@$(MAKE) -C liba52 TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR) VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEMORYSIZE}

$(OBJDIR)/libFLAC.a:
	@echo "MAKE in libFLAC"
	@mkdir -p $(OBJDIR)/libFLAC
	@$(MAKE) -C libFLAC TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR) VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEMORYSIZE}

$(OBJDIR)/libTremor.a:
	@echo "MAKE in Tremor"
	@mkdir -p $(OBJDIR)/Tremor
	@$(MAKE) -C Tremor TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR) VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEMORYSIZE}

clean:
	@echo "cleaning codecs"
	@rm -fr $(OBJDIR)/libmad $(OBJDIR)/liba52 $(OBJDIR)/libFLAC $(OBJDIR)/Tremor
	@$(MAKE) -C libmad clean
	@$(MAKE) -C liba52 clean
	@$(MAKE) -C libFLAC clean
	@$(MAKE) -C Tremor clean
