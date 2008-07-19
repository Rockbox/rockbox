#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

ifndef V
SILENT = @
endif

SPEEXSRC = ../../apps/codecs/libspeex

INCLUDES = -I $(SPEEXSRC)
SPEEXOPTS = -DHAVE_CONFIG_H -DROCKBOX_VOICE_ENCODER

CFLAGS = $(SPEEXOPTS) $(INCLUDES) -O3 -fomit-frame-pointer -Wno-unused-parameter

#build standalone win32 executables on cygwin
ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
CFLAGS+=-mno-cygwin
endif
# on mingw32, make sure CC is set to gcc
ifeq ($(findstring MINGW,$(shell uname)),MINGW)
CC = gcc
endif

ifdef ARCH
ARCH_CMD = -arch $(ARCH)
endif


# This sets up 'SRC' based on the files mentioned in SOURCES
SRC := $(shell cat $(SPEEXSRC)/SOURCES | $(CC) $(CFLAGS) -E -P - | grep -v "^\#")

SOURCES = $(SRC:%.c=$(SPEEXSRC)/%.c) rbspeex.c rbspeexenc.c rbspeexdec.c
OBJS := $(SRC:%.c=%.o)
DEPFILE = dep-speex
DIRS = 

.PHONY : all

all: ../rbspeexenc ../rbspeexdec

$(DEPFILE): $(SOURCES)
	$(SILENT)rm -f $(DEPFILE)
	$(SILENT)(for each in $(SOURCES) x; do \
	    if test "x" != "$$each"; then \
	        obj=`echo $$each | sed -e 's/\.[cS]/.o/' | sed -e 's/^.*\///' `; \
	        $(CC) -MG -MM -MT "$$obj" $(CFLAGS) $$each 2>/dev/null; \
	    fi; \
	    if test -n "$$del"; then \
	        rm $$del; \
	        del=""; \
	    fi \
	done > $(DEPFILE); \
	echo "oo" > /dev/null )

librbspeex$(ARCH).a: $(OBJS) $(DEPFILE) rbspeex.o 
	@echo AR librbspeex.a
	$(SILENT)$(AR) ruv $@ $+ > /dev/null 2>&1

../rbspeexenc: $(OBJS) rbspeexenc.o librbspeex.a 
	@echo Linking ../rbspeexenc
	$(SILENT)$(CC) $(CFLAGS) -o ../rbspeexenc rbspeexenc.o librbspeex.a -lm

../rbspeexdec: $(OBJS) librbspeex.a rbspeexdec.o 
	@echo Linking ../rbspeexdec
	$(SILENT)$(CC) $(CFLAGS) -o ../rbspeexdec rbspeexdec.o librbspeex.a -lm 

%.o:
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -c $< -o $@ $(ARCH_CMD)

universal: librbspeexppc.a librbspeexi386.a
	@echo lipo libusb.a
	lipo -create librbspeexppc.a librbspeexi386.a -output librbspeex.a

clean:
	rm -f $(OBJS) rbspeex.o libspeex.a librbspeex.a rbspeexenc.o ../rbspeexenc dep-speex

-include $(DEPFILE)
