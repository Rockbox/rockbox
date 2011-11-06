#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

#change for releases
ifndef APPVERSION
APPVERSION=$(shell ../../tools/version.sh ../../)
endif
TARGET_DIR ?= $(shell pwd)/
# We use the SB code available in the Rockbox utils/sbtools directory
IMXTOOLS_DIR=../../utils/imxtools/
CFLAGS=-I$(IMXTOOLS_DIR) -Wall -DVERSION=\"$(APPVERSION)\"

ifndef V
SILENT = @
endif

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
OUTPUT=mkimxboot.exe
CFLAGS+=-mno-cygwin
else
ifeq ($(findstring MINGW,$(shell uname)),MINGW)
OUTPUT=mkimxboot.exe
else
ifeq ($(findstring mingw,$(CC)),mingw)
OUTPUT=mkimxboot.exe
else
OUTPUT=mkimxboot
endif
endif
endif

ifdef RBARCH
CFLAGS += -arch $(RBARCH)
OBJDIR = $(TARGET_DIR)build/$(RBARCH)/
else
OBJDIR = $(TARGET_DIR)build/
endif

CFLAGS += -std=c99

all: $(OUTPUT)

# inputs
LIBIMXSOURCES=misc.c sb.c crypto.c crc.c aes128.c sha1.c
LIBSOURCES := dualboot.c mkimxboot.c md5.c $(LIBIMXSOURCES)
SOURCES := $(LIBSOURCES) main.c
OBJS := $(patsubst %.c,%.o,$(addprefix $(OBJDIR),$(SOURCES)))
LIBOBJS := $(patsubst %.c,%.o,$(addprefix $(OBJDIR),$(LIBSOURCES)))
EXTRADEPS := 

# explicit dependencies on dualboot.{c,h} and mkimxboot.h
$(OBJDIR)mkimxboot.o: dualboot.h dualboot.c mkimxboot.c mkimxboot.h
$(OBJDIR)main.o: dualboot.h dualboot.c main.c mkimxboot.h

$(OBJDIR)%.o: %.c
	@echo CC $<
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)%.o: $(IMXTOOLS_DIR)%.c
	@echo CC $<
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CC) $(CFLAGS) -c -o $@ $<

libmkimxboot$(RBARCH).a: $(TARGET_DIR)libmkimxboot$(RBARCH).a

$(TARGET_DIR)libmkimxboot$(RBARCH).a: $(LIBOBJS)
	@echo AR $(notdir $@)
	$(SILENT)$(AR) rucs $@ $^

# building the standalone executable
$(OUTPUT): $(OBJS) $(EXTRADEPS)
	@echo LD $@
	$(SILENT)$(CC) $(CFLAGS) -o $(OUTPUT) $(OBJS) $(EXTRADEPS)

# some trickery to build ppc and i386 from a single call
ifeq ($(RBARCH),)
$(TARGET_DIR)libmkimxbooti386.a:
	make RBARCH=i386 TARGET_DIR=$(TARGET_DIR) libmkimxbooti386.a

$(TARGET_DIR)libmkimxbootppc.a:
	make RBARCH=ppc TARGET_DIR=$(TARGET_DIR) libmkimxbootppc.a
endif

libmkimxboot-universal: $(TARGET_DIR)libmkimxbooti386.a $(TARGET_DIR)libmkimxbootppc.a
	@echo lipo $(TARGET_DIR)libmkimxboot.a
	$(SILENT) rm -f $(TARGET_DIR)libmkimxboot.a
	$(SILENT)lipo -create $(TARGET_DIR)libmkimxbootppc.a $(TARGET_DIR)libmkimxbooti386.a -output $(TARGET_DIR)libmkimxboot.a

clean:
	rm -f $(OBJS) $(OUTPUT) libmkimxboot.o $(TARGET_DIR)libmkimxboot*.a mkimxboot.dmg
	rm -rf mkimxboot-* i386 ppc $(OBJDIR)

mkimxboot-i386:
	$(MAKE) RBARCH=i386
	mv mkimxboot mkimxboot-i386

mkimxboot-ppc:
	$(MAKE) RBARCH=ppc
	mv mkimxboot mkimxboot-ppc

mkimxboot-mac: mkimxboot-i386 mkimxboot-ppc
	$(SILENT)lipo -create mkimxboot-ppc mkimxboot-i386 -output mkimxboot-mac

mkimxboot.dmg: mkimxboot-mac
	mkdir -p mkimxboot-dmg
	cp -p mkimxboot-mac mkimxboot-dmg
	hdiutil create -srcfolder mkimxboot-dmg mkimxboot.dmg
