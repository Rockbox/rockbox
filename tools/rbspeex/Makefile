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

ifdef RBARCH
CFLAGS += -arch $(RBARCH)
endif

# don't try to use the systems libspeex when building a static binary.
ifndef STATIC
SYS_SPEEX = $(shell pkg-config --silence-errors --libs speex speexdsp)
endif

# fall back to our own librbspeex if no suitable found.
ifeq ($(SYS_SPEEX),)
# This sets up 'SRC' based on the files mentioned in SOURCES
SRC := $(shell cat $(SPEEXSRC)/SOURCES | $(CC) $(CFLAGS) -E -P - | grep -v "^\#" | grep -v "^$$")
LIBS = $(TARGET_DIR)librbspeex$(RBARCH).a
else
LIBS = $(SYS_SPEEX)
endif

OUT = $(TARGET_DIR)build$(RBARCH)
SOURCES = $(SRC:%.c=$(SPEEXSRC)/%.c) rbspeex.c rbspeexenc.c rbspeexdec.c
OBJS = $(addprefix $(OUT)/,$(SRC:%.c=%.o))
DEPFILE = $(OUT)/dep-speex
DIRS = 

.PHONY : all

all: ../rbspeexenc ../rbspeexdec

$(DEPFILE): $(SOURCES)
	@echo MKDIR $(OUT)
	$(SILENT)mkdir -p $(OUT)
	@echo Creating dependencies
	$(SILENT)rm -f $(DEPFILE)
	$(SILENT)(for each in $(SOURCES) x; do \
	    if test "x" != "$$each"; then \
	        obj=`echo $$each | sed -e 's/\.[cS]/.o/' | sed -e 's/^.*\///' `; \
	        $(CC) -MG -MM -MT "$(OUT)/$$obj" $(CFLAGS) $$each 2>/dev/null; \
	    fi; \
	    if test -n "$$del"; then \
	        rm $$del; \
	        del=""; \
	    fi \
	done > $(DEPFILE); \
	echo "oo" > /dev/null )

-include $(DEPFILE)

$(OUT)/librbspeex.a: $(OBJS) $(DEPFILE) $(OUT)/rbspeex.o 
	@echo AR $(OUT)/librbspeex$(RBARCH).a
	$(SILENT)$(AR) ruv $@ $+ > /dev/null 2>&1

librbspeex$(RBARCH).a: $(OUT)/librbspeex.a
	$(SILENT)cp $(OUT)/librbspeex.a $(TARGET_DIR)librbspeex$(RBARCH).a

../rbspeexenc: $(OBJS) $(OUT)/rbspeexenc.o librbspeex$(RBARCH).a
	@echo Linking ../rbspeexenc
	$(SILENT)$(CC) $(CFLAGS) -o ../rbspeexenc $(OUT)/rbspeexenc.o $(LIBS) -lm $(TARGET_DIR)librbspeex$(RBARCH).a

../rbspeexdec: $(OBJS) librbspeex$(RBARCH).a $(OUT)/rbspeexdec.o 
	@echo Linking ../rbspeexdec
	$(SILENT)$(CC) $(CFLAGS) -o ../rbspeexdec $(OUT)/rbspeexdec.o $(LIBS) -lm $(TARGET_DIR)librbspeex$(RBARCH).a

%.o:
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -c $< -o $@

# some trickery to build ppc and i386 from a single call
ifeq ($(RBARCH),)
librbspeexi386.a:
	make RBARCH=i386 TARGET_DIR=$(TARGET_DIR) librbspeexi386.a

librbspeexppc.a:
	make RBARCH=ppc TARGET_DIR=$(TARGET_DIR) librbspeexppc.a
endif

librbspeex-universal: librbspeexi386.a librbspeexppc.a
	@echo lipo librbspeex.a
	$(SILENT) rm -f $(TARGET_DIR)librbspeex.a
	$(SILENT)lipo -create $(TARGET_DIR)librbspeexppc.a $(TARGET_DIR)librbspeexi386.a -output $(TARGET_DIR)librbspeex.a

clean:
	rm -f $(OBJS) $(TARGET_DIR)librbspeex* ../rbspeexenc ../rbspeexdec $(TARGET_DIR)dep-speex
	rm -rf build*

$(OUT):
	@echo MKDIR $(OUT)
	$(SILENT)mkdir $(OUT)

