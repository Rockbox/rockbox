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

all: $(OBJDIR)/libmad.a

include $(TOOLSDIR)/make.inc

$(OBJDIR)/libmad.a:
	@echo "MAKE in libmad"
	@mkdir -p $(OBJDIR)/libmad
	@$(MAKE) -C libmad TARGET=$(TARGET) DEBUG=$(DEBUG) OBJDIR=$(OBJDIR) VERSION=$(VERSION) EXTRA_DEFINES="$(EXTRA_DEFINES)" MEM=${MEMORYSIZE}

clean:
	@echo "cleaning codecs"
	@rm -fr $(OBJDIR)/libmad
	@$(MAKE) -C libmad clean
