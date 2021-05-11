#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# Shameless copy+paste from tools/ucl/src/Makefile
# This is only used for rbutil builds, Rockbox uses microtar.make

ifndef V
SILENT = @
endif

ifeq ($(OS),Windows_NT)
SHELL = cmd.exe
mkdir = if not exist $(subst /,\,$(1)) mkdir $(subst /,\,$(1))
else
mkdir = mkdir -p $(1)
endif

ifdef RBARCH
CFLAGS += -arch $(RBARCH)
endif

CPPDEFINES := $(shell echo foo | $(CROSS)$(CC) -dM -E -)
#build standalone win32 executables on cygwin
ifeq ($(findstring CYGWIN,$(CPPDEFINES)),CYGWIN)
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

TARGET_DIR ?= $(shell pwd)/
OBJDIR = $(TARGET_DIR)build$(RBARCH)

SOURCES = microtar.c microtar-stdio.c

OBJS =  $(addprefix $(OBJDIR)/,$(SOURCES:%.c=%.o))

libmicrotar$(RBARCH).a: $(TARGET_DIR)libmicrotar$(RBARCH).a

dll: microtar.dll
microtar.dll: $(TARGET_DIR)microtar.dll
$(TARGET_DIR)microtar.dll: $(OBJS)
		@echo DLL $(notdir $@)
		$(SILENT)$(CROSS)$(CC) $(CFLAGS) -shared -o $@ $^ \
					-Wl,--output-def,$(TARGET_DIR)microtar.def

$(TARGET_DIR)libmicrotar$(RBARCH).a: $(OBJS)
		@echo AR $(notdir $@)
		$(SILENT)$(CROSS)$(AR) rcs $@ $(OBJS)

$(OBJDIR)/%.o: %.c
		@echo CC $<
		$(SILENT)$(call mkdir, $(dir $@))
		$(SILENT)$(CROSS)$(CC) $(CFLAGS) $(ARCHFLAGS) -c $< -o $@

clean:
		rm -f $(TARGET_DIR)libmicrotar*.a
		rm -rf build*
