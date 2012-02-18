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
RBCODEC_CFLAGS += -D_FILE_H_ -DLOGF_H -DDEBUG_H -D_KERNEL_H_ # will be removed later

SRC = $(ROOTDIR)/lib/rbcodec/test/warble.c

INCLUDES += -I$(ROOTDIR)/lib/rbcodec/test
INCLUDES += -I$(ROOTDIR)/apps -I$(ROOTDIR)/apps/gui
INCLUDES += -I$(ROOTDIR)/firmware/export -I$(ROOTDIR)/firmware/include \
			-I$(ROOTDIR)/firmware/target/hosted \
			-I$(ROOTDIR)/firmware/target/hosted/sdl

GCCOPTS+=-D__PCTOOL__ -DRBCODEC_NOT_ROCKBOX -DDEBUG -g -std=gnu99 `$(SDLCONFIG) --cflags` -DCODECDIR="\"$(CODECDIR)\""

LIBS=`$(SDLCONFIG) --libs` -lc
ifneq ($(findstring MINGW,$(shell uname)),MINGW)
LIBS += -ldl
endif

.SECONDEXPANSION: # $$(OBJ) is not populated until after this

include $(ROOTDIR)/tools/functions.make
include $(ROOTDIR)/lib/rbcodec/rbcodec.make

$(BUILDDIR)/lib/rbcodec/test/%.o: $(ROOTDIR)/lib/rbcodec/test/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $<)$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/$(BINARY): $(CODECS)

$(BUILDDIR)/$(BINARY): $$(OBJ) $(RBCODEC_LIB)
	@echo LD $(BINARY)
	$(SILENT)$(HOSTCC) $(SIMFLAGS) $(LIBS) -o $@ $+
