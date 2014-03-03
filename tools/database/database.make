#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: checkwps.make 22680 2009-09-11 17:58:17Z gevaerts $
#

GCCOPTS += -g -DDEBUG -D__PCTOOL__ -DDBTOOL

createsrc = $(shell cat $(1) > $(3); echo "\#if CONFIG_CODEC == SWCODEC" >> $(3); \
                                     echo $(2) | sed 's/ /\n/g' >> $(3); \
                                     echo "\#endif" >> $(3); \
                                     echo $(3))

METADATAS := $(subst $(ROOTDIR), ../.., $(wildcard $(ROOTDIR)/lib/rbcodec/metadata/*.c))

SRCFILE := $(call createsrc, $(TOOLSDIR)/database/SOURCES, \
                             $(METADATAS), \
                             $(TOOLSDIR)/database/SOURCES.build)

DATABASE_SRC = $(call preprocess, $(SRCFILE))
DATABASE_OBJ = $(call c2obj,$(DATABASE_SRC))

OTHER_SRC += $(DATABASE_SRC)

INCLUDES += -I$(ROOTDIR)/apps/gui \
            -I$(FIRMDIR)/kernel/include \
            -I$(ROOTDIR)/firmware/export \
            -I$(ROOTDIR)/firmware/include \
            -I$(ROOTDIR)/apps \
            -I$(ROOTDIR)/apps/recorder \
            -I$(ROOTDIR)/lib/rbcodec \
            -I$(ROOTDIR)/lib/rbcodec/metadata \
            -I$(ROOTDIR)/lib/rbcodec/dsp \
            -I$(APPSDIR) \
            -I$(BUILDDIR)
            
ifdef SOFTWARECODECS
OTHERLIBS := $(FIXEDPOINTLIB)
endif

.SECONDEXPANSION: # $$(OBJ) is not populated until after this

$(BUILDDIR)/$(BINARY): $$(DATABASE_OBJ) $(OTHERLIBS)
	$(call PRINTS,LD $(BINARY))
	$(SILENT)$(HOSTCC) $(call a2lnk $(OTHERLIBS)) -o $@ $+
