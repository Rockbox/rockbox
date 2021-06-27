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

ifeq ($(OS),Windows_NT)
SHELL = cmd.exe
mkdir = if not exist $(subst /,\,$(1)) mkdir $(subst /,\,$(1))
rm = if exist $(subst /,\,$(1)) del /q /s $(subst /,\,$(1))
else
mkdir = mkdir -p $(1)
rm = rm -rf $(1)
endif

SPEEXSRC = ../../lib/rbcodec/codecs/libspeex

INCLUDES = -I $(SPEEXSRC)
SPEEXOPTS = -DHAVE_CONFIG_H -DROCKBOX_VOICE_ENCODER

CFLAGS += $(SPEEXOPTS) $(INCLUDES) -O3 -fomit-frame-pointer -Wno-unused-parameter

CPPDEFINES := $(shell echo foo | $(CROSS)$(CC) -dM -E -)
#build standalone win32 executables on cygwin
ifeq ($(findstring CYGWIN,$(CPPDEFINES)),CYGWIN)
CFLAGS+=-mno-cygwin
COMPILETARGET = cygwin
else
ifeq ($(findstring MINGW,$(CPPDEFINES)),MINGW)
COMPILETARGET = mingw
else
# OS X specifics. Needs to consider cross compiling for Windows.
ifeq ($(findstring APPLE,$(CPPDEFINES)),APPLE)
# When building for 10.4+ we need to use gcc. Otherwise clang is used, so use
# that to determine if we need to set arch and isysroot.
ifeq ($(findstring __clang__,$(CPPDEFINES)),__clang__)
CFLAGS += -mmacosx-version-min=10.5
ifneq ($(ISYSROOT),)
CFLAGS += -isysroot $(ISYSROOT)
endif
else
# when building libs for OS X 10.4+ build for both i386 and ppc at the same time.
# This creates fat objects, and ar can only create the archive but not operate
# on it. As a result the ar call must NOT use the u (update) flag.
ARCHFLAGS += -arch ppc -arch i386
# building against SDK 10.4 is not compatible with gcc-4.2 (default on newer Xcode)
# might need adjustment for older Xcode.
CC = gcc-4.0
CFLAGS += -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
NATIVECC ?= gcc-4.0
endif
COMPILETARGET = darwin
else
COMPILETARGET = posix
endif
endif
endif
$(info Compiler creates $(COMPILETARGET) binaries)

# don't try to use the systems libspeex when building a static binary.
ifndef STATIC
SYS_SPEEX ?= $(shell pkg-config --silence-errors --libs speex speexdsp)
endif

# fall back to our own librbspeex if no suitable found.
ifeq ($(SYS_SPEEX),)
# This sets up 'SRC' based on the files mentioned in SOURCES
SPEEXSRCS := $(shell $(CC) $(CFLAGS) -E -P -xc $(SPEEXSRC)/SOURCES)
LIBSOURCES := $(SPEEXSRCS:%.c=$(SPEEXSRC)/%.c)
LIBS = $(TARGET_DIR)librbspeex.a
else
LIBS = $(SYS_SPEEX)
endif

LIBSOURCES += rbspeex.c

TARGET_DIR ?= $(shell pwd)/
BUILD_DIR ?= $(TARGET_DIR)build$(COMPILETARGET)
OBJDIR = $(abspath $(BUILD_DIR))/
SOURCES = rbspeex.c rbspeexenc.c rbspeexdec.c
OBJS := $(addsuffix .o,$(addprefix $(OBJDIR),$(notdir $(SOURCES))))
LIBOBJS := $(addsuffix .o,$(addprefix $(OBJDIR),$(notdir $(LIBSOURCES))))

.PHONY : all

all: ../rbspeexenc ../rbspeexdec

# create dependency files. Make sure to use the same prefix as with OBJS!
$(foreach src,$(SOURCES) $(LIBSOURCES),$(eval $(addprefix $(OBJDIR),$(notdir $(src).o)): $(src)))
$(foreach src,$(SOURCES) $(LIBSOURCES),$(eval $(addprefix $(OBJDIR),$(notdir $(src).d)): $(src)))
DEPS = $(addprefix $(OBJDIR),$(addsuffix .d,$(notdir $(SOURCES) $(LIBSOURCES))))
-include $(DEPS)

dll: $(TARGET_DIR)rbspeex.dll
	
$(TARGET_DIR)rbspeex.dll: $(LIBOBJS)
	$(info DLL $(notdir $@))
	$(SILENT)$(CROSS)$(CC) $(CFLAGS) -shared -o $@ $^ \
		    -Wl,--output-def,$(TARGET_DIR)rbspeex.def

$(TARGET_DIR)librbspeex.a: $(LIBOBJS)
	$(info AR $(notdir $@))
	$(SILENT)$(call rm,$@)
	$(SILENT)$(CROSS)$(AR) rcs $@ $^

librbspeex.a: $(TARGET_DIR)librbspeex.a
	
../rbspeexenc: $(OBJS) $(TARGET_DIR)librbspeex.a
	$(info Linking ../rbspeexenc)
	$(SILENT)$(CROSS)$(CC) $(CFLAGS) $(LDFLAGS) $(ARCHFLAGS) -o ../rbspeexenc $(BUILD_DIR)/rbspeexenc.c.o \
		    $(TARGET_DIR)librbspeex.a $(LIBS) -lm

../rbspeexdec: $(OBJS) $(TARGET_DIR)librbspeex.a
	$(info Linking ../rbspeexdec)
	$(SILENT)$(CROSS)$(CC) $(CFLAGS) $(LDFLAGS) $(ARCHFLAGS) -o ../rbspeexdec $(BUILD_DIR)/rbspeexdec.c.o \
		    $(TARGET_DIR)librbspeex.a $(LIBS) -lm

# common rules
$(OBJDIR)%.c.o:
	$(info CC $<)
	$(SILENT)$(call mkdir,$(dir $@))
	$(SILENT)$(CROSS)$(CC) $(ARCHFLAGS) $(GCCFLAGS) $(CFLAGS) -MMD -c -o $@ $<

clean:
	$(call rm,$(OBJS) $(LIBOBJS) $(TARGET_DIR)librbspeex* ../rbspeexenc ../rbspeexdec)
	$(call rm,$(DEPS))
	$(call rm,$(BUILD_DIR))

$(BUILD_DIR):
	$(info MKDIR $(BUILD_DIR))
	$(SILENT)$(call mkdir, $(BUILD_DIR))

