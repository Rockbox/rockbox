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

# overwrite for releases
APPVERSION ?= $(shell $(TOP)/../tools/version.sh ../)
CFLAGS += -DVERSION=\"$(APPVERSION)\"
TARGET_DIR ?= $(shell pwd)/

# use POSIX/C99 printf on windows
CFLAGS += -D__USE_MINGW_ANSI_STDIO=1

BINARY = $(OUTPUT)
# when building a Windows binary add the correct file suffix
ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
BINARY = $(OUTPUT).exe
CFLAGS+=-mno-cygwin
else
ifeq ($(findstring MINGW,$(shell uname)),MINGW)
BINARY = $(OUTPUT).exe
else
ifeq ($(findstring mingw,$(CROSS)$(CC)),mingw)
BINARY = $(OUTPUT).exe
endif
endif
endif

NATIVECC ?= gcc
CC ?= gcc
# OS X specifics. Needs to consider cross compiling for Windows.
ifeq ($(findstring Darwin,$(shell uname)),Darwin)
ifneq ($(findstring mingw,$(CROSS)$(CC)),mingw)
# when building libs for OS X build for both i386 and ppc at the same time.
# This creates fat objects, and ar can only create the archive but not operate
# on it. As a result the ar call must NOT use the u (update) flag.
CFLAGS += -arch ppc -arch i386
# building against SDK 10.4 is not compatible with gcc-4.2 (default on newer Xcode)
# might need adjustment for older Xcode.
CC ?= gcc-4.0
CFLAGS += -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
NATIVECC ?= gcc-4.0
endif
endif
WINDRES = windres

BUILD_DIR ?= $(TARGET_DIR)build
OBJDIR = $(abspath $(BUILD_DIR)/$(RBARCH))/

ifdef RBARCH
CFLAGS += -arch $(RBARCH)
endif

all: $(BINARY)

OBJS := $(patsubst %.c,%.o,$(addprefix $(OBJDIR),$(notdir $(SOURCES))))
LIBOBJS := $(patsubst %.c,%.o,$(addprefix $(OBJDIR),$(notdir $(LIBSOURCES))))

# additional link dependencies for the standalone executable
# extra dependencies: libucl
LIBUCL = libucl$(RBARCH).a
$(LIBUCL): $(OBJDIR)$(LIBUCL)

$(OBJDIR)$(LIBUCL):
	$(SILENT)$(MAKE) -C $(TOP)/../tools/ucl/src TARGET_DIR=$(OBJDIR) $@

# building the standalone executable
$(BINARY): $(OBJS) $(EXTRADEPS) $(addprefix $(OBJDIR),$(EXTRALIBOBJS))
	@echo LD $@
#	$(SILENT)mkdir -p $(dir $@)
# EXTRADEPS need to be built into OBJDIR.
	$(SILENT)$(CROSS)$(CC) $(CFLAGS) $(LDOPTS) -o $(BINARY) \
	    $(OBJS) $(addprefix $(OBJDIR),$(EXTRADEPS)) \
	    $(addprefix $(OBJDIR),$(EXTRALIBOBJS))

# common rules
$(OBJDIR)%.o: %.c
	@echo CC $<
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CROSS)$(CC) $(CFLAGS) -c -o $@ $<

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
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CROSS)$(CC) $(CFLAGS) -shared -o $@ $^ \
		    -Wl,--output-def,$(TARGET_DIR)$(OUTPUT).def

# create lib file from objects
$(TARGET_DIR)lib$(OUTPUT)$(RBARCH).a: $(LIBOBJS) $(addprefix $(OBJDIR),$(EXTRALIBOBJS))
	@echo AR $(notdir $@)
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(AR) rcs $@ $^

clean:
	rm -f $(OBJS) $(OUTPUT) $(TARGET_DIR)lib$(OUTPUT)*.a $(OUTPUT).dmg
	rm -rf $(OUTPUT)-* i386 ppc $(OBJDIR)

# extra tools
BIN2C = $(TOP)/tools/bin2c
$(BIN2C):
	$(MAKE) -C $(TOP)/tools

# OS X specifics
$(OUTPUT).dmg: $(OUTPUT)
	@echo DMG $@
	$(SILENT)mkdir -p $(OUTPUT)-dmg
	$(SILENT)cp -p $(OUTPUT) $(OUTPUT)-dmg
	$(SILENT)hdiutil create -srcfolder $(OUTPUT)-dmg $@
