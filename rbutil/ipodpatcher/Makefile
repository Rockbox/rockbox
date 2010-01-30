CFLAGS=-Wall -W

BOOT_H = ipod1g2g.h ipod3g.h ipod4g.h ipodcolor.h ipodmini1g.h ipodmini2g.h ipodnano1g.h ipodvideo.h

# Build with "make BOOTOBJS=1" to build with embedded bootloaders and the 
# --install option and interactive mode.  You need the full set of Rockbox 
# bootloaders in this directory - download them from
# http://download.rockbox.org/bootloader/ipod/bootloaders.zip

# Releases of ipodpatcher are created with "make RELEASE=1".  This
# enables BOOTOBJS and uses the VERSION string defined in main.c

ifdef RELEASE
CFLAGS+=-DRELEASE
BOOTOBJS=1
endif

ifdef BOOTOBJS
BOOTSRC = ipod1g2g.c ipod3g.c ipod4g.c ipodcolor.c ipodmini1g.c ipodmini2g.c ipodnano1g.c ipodvideo.c ipodnano2g.c
CFLAGS += -DWITH_BOOTOBJS
endif

ifndef VERSION
VERSION=$(shell ../../tools/version.sh)
endif

CFLAGS+=-DVERSION=\"$(VERSION)\"

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
OUTPUT=ipodpatcher.exe
CROSS=
CFLAGS+=-mno-cygwin
else 
ifeq ($(findstring MINGW,$(shell uname)),MINGW)
OUTPUT=ipodpatcher.exe
CROSS=
else
OUTPUT=ipodpatcher
CROSS=i586-mingw32msvc-
endif
endif
ifeq ($(findstring Darwin,$(shell uname)),Darwin)
# building against SDK 10.4 is not compatible with gcc-4.2 (default on newer Xcode)
# might need adjustment for older Xcode.
NATIVECC ?= gcc-4.0
CFLAGS+=-framework CoreFoundation -framework IOKit -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
endif

NATIVECC ?= gcc
CC = $(CROSS)gcc
WINDRES = $(CROSS)windres

SRC = main.c ipodpatcher.c fat32format.c arc4.c

all: $(OUTPUT)

ipodpatcher: $(SRC) ipodio-posix.c $(BOOTSRC)
	$(NATIVECC) $(CFLAGS) -o ipodpatcher $(SRC) ipodio-posix.c $(BOOTSRC)
	strip ipodpatcher

ipodpatcher.exe: $(SRC) ipodio-win32.c ipodio-win32-scsi.c ipodpatcher-rc.o $(BOOTSRC)
	$(CC) $(CFLAGS) -o ipodpatcher.exe $(SRC) ipodio-win32.c ipodio-win32-scsi.c ipodpatcher-rc.o $(BOOTSRC)
	$(CROSS)strip ipodpatcher.exe

ipodpatcher-rc.o: ipodpatcher.rc ipodpatcher.manifest
	$(WINDRES) -i ipodpatcher.rc -o ipodpatcher-rc.o

ipodpatcher-mac: ipodpatcher-i386 ipodpatcher-ppc
	lipo -create ipodpatcher-ppc ipodpatcher-i386 -output ipodpatcher-mac

ipodpatcher-i386: $(SRC) ipodio-posix.c $(BOOTSRC)
	$(NATIVECC) -arch i386 $(CFLAGS) -o ipodpatcher-i386 $(SRC) ipodio-posix.c $(BOOTSRC)
	strip ipodpatcher-i386

ipodpatcher-ppc: $(SRC) ipodio-posix.c $(BOOTSRC)
	$(NATIVECC) -arch ppc $(CFLAGS) -o ipodpatcher-ppc $(SRC) ipodio-posix.c $(BOOTSRC)
	strip ipodpatcher-ppc

ipod2c: ipod2c.c
	$(NATIVECC) $(CFLAGS) -o ipod2c ipod2c.c

ipod1g2g.c: bootloader-ipod1g2g.ipod ipod2c
	./ipod2c bootloader-ipod1g2g.ipod ipod1g2g

ipod3g.c: bootloader-ipod3g.ipod ipod2c
	./ipod2c bootloader-ipod3g.ipod ipod3g

ipod4g.c: bootloader-ipod4g.ipod ipod2c
	./ipod2c bootloader-ipod4g.ipod ipod4g

ipodcolor.c: bootloader-ipodcolor.ipod ipod2c
	./ipod2c bootloader-ipodcolor.ipod ipodcolor

ipodmini1g.c: bootloader-ipodmini1g.ipod ipod2c
	./ipod2c bootloader-ipodmini1g.ipod ipodmini1g

ipodmini2g.c: bootloader-ipodmini2g.ipod ipod2c
	./ipod2c bootloader-ipodmini2g.ipod ipodmini2g

ipodnano1g.c: bootloader-ipodnano1g.ipod ipod2c
	./ipod2c bootloader-ipodnano1g.ipod ipodnano1g

ipodvideo.c: bootloader-ipodvideo.ipod ipod2c
	./ipod2c bootloader-ipodvideo.ipod ipodvideo

ipodnano2g.c: bootloader-ipodnano2g.ipodx ipod2c
	./ipod2c bootloader-ipodnano2g.ipodx ipodnano2g


clean:
	rm -f ipodpatcher.exe ipodpatcher-rc.o ipodpatcher-mac ipodpatcher-i386 ipodpatcher-ppc ipodpatcher ipod2c *~ $(BOOTSRC) $(BOOT_H)
