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

SPEEXSRC = ../../lib/rbcodec/codecs/libspeex

INCLUDES = -I $(SPEEXSRC)
SPEEXOPTS = -DHAVE_CONFIG_H -DROCKBOX_VOICE_ENCODER

CFLAGS += $(SPEEXOPTS) $(INCLUDES) -O3 -fomit-frame-pointer -Wno-unused-parameter

#build standalone win32 executables on cygwin
ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
CFLAGS+=-mno-cygwin
endif
# OS X specifics. Needs to consider cross compiling for Windows.
ifeq ($(findstring Darwin,$(shell uname)),Darwin)
ifneq ($(findstring mingw,$(CROSS)$(CC)),mingw)
# when building libs for OS X build for both i386 and ppc at the same time.
# This creates fat objects, and ar can only create the archive but not operate
# on it. As a result the ar call must NOT use the u (update) flag.
ARCHFLAGS = -arch ppc -arch i386
# building against SDK 10.4 is not compatible with gcc-4.2 (default on newer Xcode)
# might need adjustment for older Xcode.
CC ?= gcc-4.0
CFLAGS += -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
NATIVECC ?= gcc-4.0
endif
endif

# don't try to use the systems libspeex when building a static binary.
ifndef STATIC
SYS_SPEEX ?= $(shell pkg-config --silence-errors --libs speex speexdsp)
endif

# fall back to our own librbspeex if no suitable found.
ifeq ($(SYS_SPEEX),)
# This sets up 'SRC' based on the files mentioned in SOURCES
SRC := $(shell cat $(SPEEXSRC)/SOURCES | $(CC) $(CFLAGS) -E -P - | grep -v "^\#" | grep -v "^$$")
LIBS = $(TARGET_DIR)librbspeex.a
else
LIBS = $(SYS_SPEEX)
endif

TARGET_DIR ?= $(shell pwd)/
BUILD_DIR ?= $(TARGET_DIR)build
SOURCES = $(SRC:%.c=$(SPEEXSRC)/%.c) rbspeex.c rbspeexenc.c rbspeexdec.c
OBJS = $(addprefix $(BUILD_DIR)/,$(SRC:%.c=%.o))
DEPFILE = $(BUILD_DIR)/dep-speex
DIRS = 

.PHONY : all

all: ../rbspeexenc ../rbspeexdec

$(DEPFILE): $(SOURCES)
	@echo MKDIR $(BUILD_DIR)
	$(SILENT)mkdir -p $(BUILD_DIR)
	@echo Creating dependencies
	$(SILENT)rm -f $(DEPFILE)
	$(SILENT)(for each in $(SOURCES) x; do \
	    if test "x" != "$$each"; then \
	        obj=`echo $$each | sed -e 's/\.[cS]/.o/' | sed -e 's/^.*\///' `; \
	        $(CC) -MG -MM -MT "$(BUILD_DIR)/$$obj" $(CFLAGS) $$each 2>/dev/null; \
	    fi; \
	    if test -n "$$del"; then \
	        rm $$del; \
	        del=""; \
	    fi \
	done > $(DEPFILE); \
	echo "oo" > /dev/null )

include $(DEPFILE)

dll: $(TARGET_DIR)rbspeex.dll
	
$(TARGET_DIR)rbspeex.dll: $(OBJS) $(BUILD_DIR)/rbspeex.o
	@echo DLL $(notdir $@)
	$(SILENT)$(CROSS)$(CC) $(CFLAGS) -shared -o $@ $^ \
		    -Wl,--output-def,$(TARGET_DIR)rbspeex.def

$(TARGET_DIR)librbspeex.a: $(OBJS) $(BUILD_DIR)/rbspeex.o
	@echo AR $(notdir $@)
	$(SILENT)rm -f $@
	$(SILENT)$(CROSS)$(AR) rcs $@ $^ > /dev/null 2>&1

librbspeex.a: $(TARGET_DIR)librbspeex.a
	
../rbspeexenc: $(OBJS) $(BUILD_DIR)/rbspeexenc.o librbspeex.a
	@echo Linking ../rbspeexenc
	$(SILENT)$(CROSS)$(CC) $(CFLAGS) $(ARCHFLAGS) -o ../rbspeexenc $(BUILD_DIR)/rbspeexenc.o \
		    $(LIBS) -lm $(TARGET_DIR)librbspeex.a

../rbspeexdec: $(OBJS) librbspeex.a $(BUILD_DIR)/rbspeexdec.o
	@echo Linking ../rbspeexdec
	$(SILENT)$(CROSS)$(CC) $(CFLAGS) $(ARCHFLAGS) -o ../rbspeexdec $(BUILD_DIR)/rbspeexdec.o \
		    $(LIBS) -lm $(TARGET_DIR)librbspeex.a

%.o:
	@echo CC $<
	$(SILENT)$(CROSS)$(CC) $(CFLAGS) $(ARCHFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET_DIR)librbspeex* ../rbspeexenc ../rbspeexdec $(TARGET_DIR)dep-speex
	rm -rf build*

$(BUILD_DIR):
	@echo MKDIR $(BUILD_DIR)
	$(SILENT)mkdir $(BUILD_DIR)

