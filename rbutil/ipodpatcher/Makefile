CFLAGS=-Wall -W

BOOT_H = ipod1g2g.h ipod3g.h ipod4g.h ipodcolor.h ipodmini.h ipodmini2g.h ipodnano.h ipodvideo.h

# Uncomment the next two lines to build with embedded bootloaders and the 
# --install option and interactive mode.  You need the full set of Rockbox 
# bootloaders in this directory - download them from
# http://download.rockbox.org/bootloader/ipod/bootloaders.zip

#BOOTSRC = ipod1g2g.c ipod3g.c ipod4g.c ipodcolor.c ipodmini.c ipodmini2g.c ipodnano.c ipodvideo.c
#CFLAGS += -DWITH_BOOTOBJS

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

NATIVECC = gcc
CC = $(CROSS)gcc
WINDRES = $(CROSS)windres

SRC = main.c ipodpatcher.c fat32format.c parttypes.h arc4.c

all: $(OUTPUT)

ipodpatcher: $(SRC) ipodio-posix.c $(BOOTSRC)
	gcc $(CFLAGS) -o ipodpatcher $(SRC) ipodio-posix.c $(BOOTSRC)
	strip ipodpatcher

ipodpatcher.exe: $(SRC) ipodio-win32.c ipodpatcher-rc.o $(BOOTSRC)
	$(CC) $(CFLAGS) -o ipodpatcher.exe $(SRC) ipodio-win32.c ipodpatcher-rc.o $(BOOTSRC)
	$(CROSS)strip ipodpatcher.exe

ipodpatcher-rc.o: ipodpatcher.rc ipodpatcher.manifest
	$(WINDRES) -i ipodpatcher.rc -o ipodpatcher-rc.o

ipodpatcher-mac: ipodpatcher-i386 ipodpatcher-ppc
	lipo -create ipodpatcher-ppc ipodpatcher-i386 -output ipodpatcher-mac

ipodpatcher-i386: $(SRC) ipodio-posix.c $(BOOTSRC)
	gcc -isysroot /Developer/SDKs/MacOSX10.4u.sdk -o bin/i386/program -arch i386 $(CFLAGS) -o ipodpatcher-i386 $(SRC) ipodio-posix.c $(BOOTSRC)
	strip ipodpatcher-i386

ipodpatcher-ppc: $(SRC) ipodio-posix.c $(BOOTSRC)
	gcc -arch ppc $(CFLAGS) -o ipodpatcher-ppc $(SRC) ipodio-posix.c $(BOOTSRC)
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

ipodmini.c: bootloader-ipodmini.ipod ipod2c
	./ipod2c bootloader-ipodmini.ipod ipodmini

ipodmini2g.c: bootloader-ipodmini2g.ipod ipod2c
	./ipod2c bootloader-ipodmini2g.ipod ipodmini2g

ipodnano.c: bootloader-ipodnano.ipod ipod2c
	./ipod2c bootloader-ipodnano.ipod ipodnano

ipodvideo.c: bootloader-ipodvideo.ipod ipod2c
	./ipod2c bootloader-ipodvideo.ipod ipodvideo


clean:
	rm -f ipodpatcher.exe ipodpatcher-rc.o ipodpatcher-mac ipodpatcher-i386 ipodpatcher-ppc ipodpatcher ipod2c *~ $(BOOTSRC) $(BOOT_H)
