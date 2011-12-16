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
ifndef APPVERSION
APPVERSION=$(shell $(TOP)/../tools/version.sh ../)
endif
CFLAGS += -DVERSION=\"$(APPVERSION)\"
TARGET_DIR = $(shell pwd)/

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

NATIVECC = gcc
CC = gcc
ifeq ($(findstring Darwin,$(shell uname)),Darwin)
# building against SDK 10.4 is not compatible with gcc-4.2 (default on newer Xcode)
# might need adjustment for older Xcode.
CC = gcc-4.0
CFLAGS += -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
NATIVECC = gcc-4.0
endif
WINDRES = windres

ifndef BUILD_DIR
BUILD_DIR = $(TARGET_DIR)build
endif
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

$(TARGET_DIR)lib$(OUTPUT)$(RBARCH).a: $(LIBOBJS) $(addprefix $(OBJDIR),$(EXTRALIBOBJS))
	@echo AR $(notdir $@)
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(AR) rucs $@ $^

# some trickery to build ppc and i386 from a single call
ifeq ($(RBARCH),)
$(TARGET_DIR)lib$(OUTPUT)i386.a:
	make RBARCH=i386 TARGET_DIR=$(TARGET_DIR) lib$(OUTPUT)i386.a

$(TARGET_DIR)lib$(OUTPUT)ppc.a:
	make RBARCH=ppc TARGET_DIR=$(TARGET_DIR) lib$(OUTPUT)ppc.a
endif

lib$(OUTPUT)-universal: $(TARGET_DIR)lib$(OUTPUT)i386.a \
			$(TARGET_DIR)lib$(OUTPUT)ppc.a
	@echo LIPO $(notdir $(TARGET_DIR)lib$(OUTPUT).a)
	$(SILENT) rm -f $(TARGET_DIR)lib$(OUTPUT).a
	$(SILENT)lipo -create $(TARGET_DIR)lib$(OUTPUT)i386.a \
			      $(TARGET_DIR)lib$(OUTPUT)ppc.a \
			      -output $(TARGET_DIR)lib$(OUTPUT).a

clean:
	rm -f $(OBJS) $(OUTPUT) $(TARGET_DIR)lib$(OUTPUT)*.a $(OUTPUT).dmg
	rm -rf $(OUTPUT)-* i386 ppc $(OBJDIR)

# OS X specifics
$(OUTPUT)-i386:
	$(MAKE) RBARCH=i386
	mv $(OUTPUT) $@

$(OUTPUT)-ppc:
	$(MAKE) RBARCH=ppc
	mv $(OUTPUT) $@

$(OUTPUT)-mac: $(OUTPUT)-i386 $(OUTPUT)-ppc
	@echo LIPO $@
	$(SILENT)lipo -create $(OUTPUT)-ppc $(OUTPUT)-i386 -output $@

$(OUTPUT).dmg: $(OUTPUT)-mac
	@echo DMG $@
	$(SILENT)mkdir -p $(OUTPUT)-dmg
	$(SILENT)cp -p $(OUTPUT)-mac $(OUTPUT)-dmg
	$(SILENT)hdiutil create -srcfolder $(OUTPUT)-dmg $@
