#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

# libtools.make
#
# Common Makefile for tools used by Rockbox Utility.
# Provides rules for building as library, universal library (OS X) and
# standalone binary.
#
# LIBSOURCES is a list of files to build the lib
# SOURCES is a list of files to build for the main binary
# EXTRADEPS is a list of make targets dependencies
#
ifndef V
SILENT = @
endif

# Get directory this Makefile is in for relative paths.
TOP := $(dir $(lastword $(MAKEFILE_LIST)))
ifeq ($(OS),Windows_NT)
mkdir = if not exist $(subst /,\,$(1)) mkdir $(subst /,\,$(1))
rm = if exist $(subst /,\,$(1)) del /q /s $(subst /,\,$(1))
else
mkdir = mkdir -p $(1)
rm = rm -rf $(1)
endif

# overwrite for releases
APPVERSION ?= $(shell $(TOP)/../tools/version.sh $(TOP)/..)
CFLAGS += -DVERSION=\"$(APPVERSION)\"
TARGET_DIR ?= $(abspath .)/

NATIVECC ?= gcc
CC ?= gcc
CPPDEFINES := $(shell echo foo | $(CROSS)$(CC) -dM -E -)
# use POSIX/C99 printf on windows
CFLAGS += -D__USE_MINGW_ANSI_STDIO=1

BINARY = $(OUTPUT)
# when building a Windows binary add the correct file suffix
ifeq ($(findstring CYGWIN,$(CPPDEFINES)),CYGWIN)
BINARY = $(OUTPUT).exe
CFLAGS+=-mno-cygwin
COMPILETARGET = cygwin
else
ifeq ($(findstring MINGW,$(CPPDEFINES)),MINGW)
BINARY = $(OUTPUT).exe
COMPILETARGET = mingw
else
ifeq ($(findstring APPLE,$(CPPDEFINES)),APPLE)
COMPILETARGET = darwin
LDOPTS += $(LDOPTS_OSX)
else
COMPILETARGET = posix
endif
endif
endif
$(info Compiler creates $(COMPILETARGET) binaries)

# OS X specifics. Needs to consider cross compiling for Windows.
ifeq ($(findstring APPLE,$(CPPDEFINES)),APPLE)
# When building for 10.4+ we need to use gcc. Otherwise clang is used, so use
# that to determine if we need to set arch and isysroot.
ifeq ($(findstring __clang__,$(CPPDEFINES)),__clang__)
CFLAGS += -mmacosx-version-min=10.5
else
# when building libs for OS X 10.4+ build for both i386 and ppc at the same time.
# This creates fat objects, and ar can only create the archive but not operate
# on it. As a result the ar call must NOT use the u (update) flag.
ARCHFLAGS += -arch ppc -arch i386
# building against SDK 10.4 is not compatible with gcc-4.2 (default on newer Xcode)
# might need adjustment for older Xcode.
CC = gcc-4.0
CFLAGS += -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
NATIVECC = gcc-4.0
endif
endif
WINDRES = windres

BUILD_DIR ?= $(TARGET_DIR)build$(COMPILETARGET)

ifdef RBARCH
ARCHFLAGS += -arch $(RBARCH)
OBJDIR = $(abspath $(BUILD_DIR)/$(RBARCH))/
else
OBJDIR = $(abspath $(BUILD_DIR))/
endif

all: $(BINARY)

OBJS := $(patsubst %.c,%.o,$(addprefix $(OBJDIR),$(notdir $(SOURCES))))
LIBOBJS := $(patsubst %.c,%.o,$(addprefix $(OBJDIR),$(notdir $(LIBSOURCES))))

# create dependency files. Make sure to use the same prefix as with OBJS!
$(foreach src,$(SOURCES) $(LIBSOURCES),$(eval $(addprefix $(OBJDIR),$(subst .c,.o,$(notdir $(src)))): $(src)))
$(foreach src,$(SOURCES) $(LIBSOURCES),$(eval $(addprefix $(OBJDIR),$(subst .c,.d,$(notdir $(src)))): $(src)))
DEPS = $(addprefix $(OBJDIR),$(subst .c,.d,$(notdir $(SOURCES) $(LIBSOURCES))))
-include $(DEPS)

# additional link dependencies for the standalone executable
# extra dependencies: libucl
LIBUCL = libucl$(RBARCH).a
$(LIBUCL): $(OBJDIR)$(LIBUCL)

$(OBJDIR)$(LIBUCL):
	$(SILENT)$(MAKE) -C $(TOP)/../tools/ucl/src TARGET_DIR=$(OBJDIR) $@

# building the standalone executable
$(BINARY): $(OBJS) $(EXTRADEPS) $(addprefix $(OBJDIR),$(EXTRALIBOBJS))
	@echo LD $@
	$(SILENT)$(call mkdir,$(dir $@))
# EXTRADEPS need to be built into OBJDIR.
	$(SILENT)$(CROSS)$(CC) $(ARCHFLAGS) $(CFLAGS) -o $(BINARY) \
	    $(OBJS) $(addprefix $(OBJDIR),$(EXTRADEPS)) \
	    $(addprefix $(OBJDIR),$(EXTRALIBOBJS)) $(LDOPTS)

# common rules
$(OBJDIR)%.o:
	@echo CC $<
	$(SILENT)$(call mkdir,$(dir $@))
	$(SILENT)$(CROSS)$(CC) $(ARCHFLAGS) $(CFLAGS) -c -o $@ $<

# lib rules
lib$(OUTPUT)$(RBARCH).a: $(TARGET_DIR)lib$(OUTPUT)$(RBARCH).a
lib$(OUTPUT)$(RBARCH): $(TARGET_DIR)lib$(OUTPUT)$(RBARCH).a

$(TARGET_DIR)lib$(OUTPUT)$(RBARCH).a: $(LIBOBJS) \
				      $(addprefix $(OBJDIR),$(EXTRALIBOBJS))
# rules to build a DLL. Only works for W32 as target (i.e. MinGW toolchain)
dll: $(OUTPUT).dll
$(OUTPUT).dll: $(TARGET_DIR)$(OUTPUT).dll
$(TARGET_DIR)$(OUTPUT).dll: $(LIBOBJS) $(addprefix $(OBJDIR),$(EXTRALIBOBJS))
	@echo DLL $(notdir $@)
	$(SILENT)$(call mkdir,$(dir $@))
	$(SILENT)$(CROSS)$(CC) $(ARCHFLAGS) $(CFLAGS) -shared -o $@ $^ \
		    -Wl,--output-def,$(TARGET_DIR)$(OUTPUT).def

# create lib file from objects
$(TARGET_DIR)lib$(OUTPUT)$(RBARCH).a: $(LIBOBJS) $(addprefix $(OBJDIR),$(EXTRALIBOBJS))
	@echo AR $(notdir $@)
	$(SILENT)$(call mkdir,$(dir $@))
	$(SILENT)$(call rm,$@)
	$(SILENT)$(AR) rcs $@ $^

clean:
	$(call rm, $(OBJS) $(OUTPUT) $(TARGET_DIR)lib$(OUTPUT)*.a $(OUTPUT).dmg)
	$(call rm, $(OUTPUT)-* i386 ppc $(OBJDIR))

%.d:
	$(SILENT)$(call mkdir,$(BUILD_DIR))
	$(SILENT)$(CC) -MG -MM -MT $(subst .d,.o,$@) $(CFLAGS) -o $(BUILD_DIR)/$(notdir $@) $<

# extra tools
BIN2C = $(TOP)/tools/bin2c
$(BIN2C):
	$(MAKE) -C $(TOP)/tools

# OS X specifics
$(OUTPUT).dmg: $(OUTPUT)
	@echo DMG $@
	$(SILENT)$(call mkdir,$(OUTPUT)-dmg))
	$(SILENT)cp -p $(OUTPUT) $(OUTPUT)-dmg
	$(SILENT)hdiutil create -srcfolder $(OUTPUT)-dmg $@
