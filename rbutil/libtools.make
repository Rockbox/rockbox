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
GCCFLAGS += -DVERSION=\"$(APPVERSION)\"
TARGET_DIR ?= $(abspath .)/

CC := gcc
CXX := g++
# use either CC or CXX to link: this makes sure the compiler knows about its
# internal dependencies. Use CXX if we have at least one c++ file, since we
# then need to link the c++ standard library (which CXX does for us).
ifeq ($(strip $(filter %.cpp,$(SOURCES) $(LIBSOURCES))),)
LD := $(CC)
else
LD := $(CXX)
endif
CPPDEFINES := $(shell echo foo | $(CROSS)$(CC) -dM -E -)

BINARY = $(OUTPUT)
# when building a Windows binary add the correct file suffix
ifeq ($(findstring CYGWIN,$(CPPDEFINES)),CYGWIN)
BINARY = $(OUTPUT).exe
GCCFLAGS += -mno-cygwin
COMPILETARGET = cygwin
else
ifeq ($(findstring MINGW,$(CPPDEFINES)),MINGW)
BINARY = $(OUTPUT).exe
COMPILETARGET = mingw
# use POSIX/C99 printf on windows
GCCFLAGS += -D__USE_MINGW_ANSI_STDIO=1
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
GCCFLAGS += -mmacosx-version-min=10.5
ifneq ($(ISYSROOT),)
GCCFLAGS += -isysroot $(ISYSROOT)
endif
else
# when building libs for OS X 10.4+ build for both i386 and ppc at the same time.
# This creates fat objects, and ar can only create the archive but not operate
# on it. As a result the ar call must NOT use the u (update) flag.
ARCHFLAGS += -arch ppc -arch i386
# building against SDK 10.4 is not compatible with gcc-4.2 (default on newer Xcode)
# might need adjustment for older Xcode.
CC = gcc-4.0
GCCFLAGS += -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
endif
endif

BUILD_DIR ?= $(TARGET_DIR)build$(COMPILETARGET)
OBJDIR = $(abspath $(BUILD_DIR))/

all: $(BINARY)

OBJS := $(addsuffix .o,$(addprefix $(OBJDIR),$(notdir $(SOURCES))))
LIBOBJS := $(addsuffix .o,$(addprefix $(OBJDIR),$(notdir $(LIBSOURCES))))

# create dependency files. Make sure to use the same prefix as with OBJS!
$(foreach src,$(SOURCES) $(LIBSOURCES),$(eval $(addprefix $(OBJDIR),$(notdir $(src).o)): $(src)))
$(foreach src,$(SOURCES) $(LIBSOURCES),$(eval $(addprefix $(OBJDIR),$(notdir $(src).d)): $(src)))
DEPS = $(addprefix $(OBJDIR),$(addsuffix .d,$(notdir $(SOURCES) $(LIBSOURCES))))
-include $(DEPS)

# additional link dependencies for the standalone executable
# extra dependencies: libucl
LIBUCL = libucl.a
$(LIBUCL): $(OBJDIR)$(LIBUCL)

$(OBJDIR)$(LIBUCL):
	$(SILENT)$(MAKE) -C $(TOP)/../tools/ucl/src TARGET_DIR=$(OBJDIR) CC=$(CC) $@

LIBBZIP2 = libbzip2.a
$(LIBBZIP2): $(OBJDIR)$(LIBBZIP2)
$(OBJDIR)$(LIBBZIP2):
	$(SILENT)$(MAKE) -C $(TOP)/bzip2 TARGET_DIR=$(OBJDIR) CC=$(CC) $@

# building the standalone executable
$(BINARY): $(OBJS) $(EXTRADEPS) $(addprefix $(OBJDIR),$(EXTRALIBOBJS)) $(TARGET_DIR)lib$(OUTPUT).a
	$(info LD $@)
	$(SILENT)$(call mkdir,$(dir $@))
# EXTRADEPS need to be built into OBJDIR.
	$(SILENT)$(CROSS)$(LD) $(ARCHFLAGS) $(LDFLAGS) -o $(BINARY) \
	    $(OBJS) $(addprefix $(OBJDIR),$(EXTRADEPS)) \
	    $(addprefix $(OBJDIR),$(EXTRALIBOBJS)) lib$(OUTPUT).a $(addprefix $(OBJDIR),$(EXTRADEPS)) $(LDOPTS)

# common rules
$(OBJDIR)%.c.o:
	$(info CC $<)
	$(SILENT)$(call mkdir,$(dir $@))
	$(SILENT)$(CROSS)$(CC) $(ARCHFLAGS) $(GCCFLAGS) $(CFLAGS) -MMD -c -o $@ $<

$(OBJDIR)%.cpp.o:
	$(info CXX $<)
	$(SILENT)$(call mkdir,$(dir $@))
	$(SILENT)$(CROSS)$(CXX) $(ARCHFLAGS) $(GCCFLAGS) $(CXXFLAGS) -MMD -c -o $@ $<

# lib rules
lib$(OUTPUT).a: $(TARGET_DIR)lib$(OUTPUT).a
lib$(OUTPUT): $(TARGET_DIR)lib$(OUTPUT).a

$(TARGET_DIR)lib$(OUTPUT).a: $(LIBOBJS) \
				      $(addprefix $(OBJDIR),$(EXTRALIBOBJS))
# rules to build a DLL. Only works for W32 as target (i.e. MinGW toolchain)
dll: $(OUTPUT).dll
$(OUTPUT).dll: $(TARGET_DIR)$(OUTPUT).dll
$(TARGET_DIR)$(OUTPUT).dll: $(LIBOBJS) $(addprefix $(OBJDIR),$(EXTRALIBOBJS))
	$(info DLL $(notdir $@))
	$(SILENT)$(call mkdir,$(dir $@))
	$(SILENT)$(CROSS)$(CC) $(ARCHFLAGS) $(GCCFLAGS) -shared -o $@ $^ \
		    -Wl,--output-def,$(TARGET_DIR)$(OUTPUT).def

# create lib file from objects
$(TARGET_DIR)lib$(OUTPUT).a: $(LIBOBJS) $(addprefix $(OBJDIR),$(EXTRALIBOBJS))
	$(info AR $(notdir $@))
	$(SILENT)$(call mkdir,$(dir $@))
	$(SILENT)$(call rm,$@)
	$(SILENT)$(CROSS)$(AR) rcs $@ $^

clean:
	$(call rm, $(OBJS) $(OUTPUT) $(TARGET_DIR)lib$(OUTPUT)*.a $(OUTPUT).dmg)
	$(call rm, $(OUTPUT)-* i386 ppc $(OBJDIR))

# extra tools
BIN2C = $(TOP)/tools/bin2c
$(BIN2C):
	$(MAKE) -C $(TOP)/tools

# OS X specifics
$(OUTPUT).dmg: $(OUTPUT)
	$(info DMG $@)
	$(SILENT)$(call mkdir,"$(OUTPUT)-$(APPVERSION)")
	$(SILENT)cp -p $(OUTPUT) "$(OUTPUT)-$(APPVERSION)"
	$(SILENT)hdiutil create -srcfolder "$(OUTPUT)-$(APPVERSION)" $@
