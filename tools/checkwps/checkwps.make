#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

GCCOPTS=-g -D__PCTOOL__ -DCHECKWPS $(TARGET)

CHECKWPS_SRC = $(call preprocess, $(TOOLSDIR)/checkwps/SOURCES)
CHECKWPS_OBJ = $(call c2obj,$(CHECKWPS_SRC))

OTHER_SRC += $(CHECKWPS_SRC)

INCLUDES = -I$(ROOTDIR)/apps/gui \
           -I$(ROOTDIR)/apps/gui/skin_engine \
           -I$(ROOTDIR)/firmware/export \
           -I$(ROOTDIR)/firmware/include \
           -I$(ROOTDIR)/firmware/target/hosted \
           -I$(ROOTDIR)/firmware/target/hosted/sdl \
           -I$(ROOTDIR)/apps \
           -I$(ROOTDIR)/apps/recorder \
           -I$(ROOTDIR)/apps/radio \
           -I$(ROOTDIR)/lib/rbcodec \
           -I$(ROOTDIR)/lib/rbcodec/metadata \
           -I$(ROOTDIR)/lib/rbcodec/dsp \
           -I$(APPSDIR) \
           -I$(BUILDDIR)

.SECONDEXPANSION: # $$(OBJ) is not populated until after this

$(BUILDDIR)/$(BINARY): $$(CHECKWPS_OBJ) $$(CORE_LIBS)
	@echo LD $(BINARY)
	$(SILENT)$(HOSTCC) -o $@ $+ $(INCLUDE) $(GCCOPTS)  \
	-L$(BUILDDIR)/lib $(call a2lnk,$(CORE_LIBS))
