#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#



RBCODEC_DIR = $(ROOTDIR)/lib/rbcodec
RBCODEC_BLD = $(BUILDDIR)/lib/rbcodec

FLAGS=-g -D__PCTOOL__ $(TARGET) -Wall

SRC= $(call preprocess, $(ROOTDIR)/lib/rbcodec/test/SOURCES)

INCLUDES += -I$(ROOTDIR)/apps -I$(ROOTDIR)/apps/codecs -I$(ROOTDIR)/apps/codecs/lib \
			-I$(ROOTDIR)/apps/gui
INCLUDES += -I$(ROOTDIR)/firmware/export -I$(ROOTDIR)/firmware/include \
			-I$(ROOTDIR)/firmware/target/hosted \
			-I$(ROOTDIR)/firmware/target/hosted/sdl

GCCOPTS+=-D__PCTOOL__ -DDEBUG -g -std=gnu99 `$(SDLCONFIG) --cflags` -DCODECDIR="\"$(CODECDIR)\""

LIBS=`$(SDLCONFIG) --libs` -lc
ifneq ($(findstring MINGW,$(shell uname)),MINGW)
LIBS += -ldl
endif

.SECONDEXPANSION: # $$(OBJ) is not populated until after this

include $(ROOTDIR)/tools/functions.make
include $(ROOTDIR)/apps/codecs/codecs.make
include $(ROOTDIR)/lib/rbcodec/rbcodec.make

$(BUILDDIR)/$(BINARY): $(CODECS)

$(BUILDDIR)/$(BINARY): $$(OBJ) $(RBCODEC_LIB)
	@echo LD $(BINARY)
	$(SILENT)$(HOSTCC) $(SIMFLAGS) $(LIBS) -o $@ $+
