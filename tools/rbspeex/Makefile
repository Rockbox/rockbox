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

# This sets up 'SRC' based on the files mentioned in SOURCES
SRC := $(shell cat $(SPEEXSRC)/SOURCES | $(CC) $(CFLAGS) -E -P - | grep -v "^\#")

SOURCES = $(SRC:%.c=$(SPEEXSRC)/%.c) rbspeexenc.c
OBJS := $(SRC:%.c=%.o)
DEPFILE = dep-speex
DIRS = 

.PHONY : all

all: ../rbspeexenc

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

libspeex.a: $(OBJS) $(DEPFILE)
	@echo AR libspeex.a
	$(SILENT)$(AR) ruv $@ $+ > /dev/null 2>&1

../rbspeexenc: $(OBJS) libspeex.a rbspeexenc.o
	@echo Linking ../rbspeexenc
	$(SILENT)$(CC) $(CFLAGS) -o ../rbspeexenc rbspeexenc.o libspeex.a -lm

%.o:
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) libspeex.a rbspeexenc.o ../rbspeexenc dep-speex

-include $(DEPFILE)
