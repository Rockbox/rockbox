#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#



FLAGS=-g -D__PCTOOL__ $(TARGET) -Wall

SRC= $(call preprocess, $(ROOTDIR)/lib/rbcodec/test/SOURCES)

INCLUDES += -I$(ROOTDIR)/apps -I$(ROOTDIR)/apps/codecs -I$(ROOTDIR)/apps/codecs/lib \
			-I$(ROOTDIR)/apps/gui -I$(ROOTDIR)/apps/metadata
INCLUDES += -I$(ROOTDIR)/firmware/export -I$(ROOTDIR)/firmware/include \
			-I$(ROOTDIR)/firmware/target/hosted \
			-I$(ROOTDIR)/firmware/target/hosted/sdl

GCCOPTS+=-D__PCTOOL__ -g -std=gnu99 `$(SDLCONFIG) --cflags` -DCODECDIR="\"$(CODECDIR)\""

LIBS=`$(SDLCONFIG) --libs` -lc
ifneq ($(findstring MINGW,$(shell uname)),MINGW)
LIBS += -ldl
endif

.SECONDEXPANSION: # $$(OBJ) is not populated until after this

include $(ROOTDIR)/tools/functions.make
include $(ROOTDIR)/apps/codecs/codecs.make

$(BUILDDIR)/$(BINARY): $(CODECS)

$(BUILDDIR)/$(BINARY): $$(OBJ)
	@echo LD $(BINARY)
	$(SILENT)$(HOSTCC) $(SIMFLAGS) $(LIBS) -o $@ $+
