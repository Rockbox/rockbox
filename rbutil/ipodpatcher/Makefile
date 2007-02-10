CFLAGS=-Wall

BOOT_H = ipod3g.h ipod4g.h ipodcolor.h ipodmini.h ipodmini2g.h ipodnano.h ipodvideo.h

# Uncomment the next two lines to build with embedded bootloaders and the 
# --install option and interactive mode.  You need the full set of Rockbox 
# bootloaders in this directory - download them from
# http://download.rockbox.org/bootloader/ipod/bootloaders.zip

#BOOTSRC = ipod3g.c ipod4g.c ipodcolor.c ipodmini.c ipodmini2g.c ipodnano.c ipodvideo.c
#CFLAGS += -DWITH_BOOTOBJS

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
OUTPUT=ipodpatcher.exe
CROSS=
CFLAGS+=-mno-cygwin
else
OUTPUT=ipodpatcher
CROSS=i586-mingw32msvc-
endif

NATIVECC = gcc
CC = $(CROSS)gcc

all: $(OUTPUT)

ipodpatcher: main.c ipodpatcher.c ipodio-posix.c parttypes.h $(BOOTSRC)
	gcc $(CFLAGS) -o ipodpatcher main.c ipodpatcher.c ipodio-posix.c $(BOOTSRC)
	strip ipodpatcher

ipodpatcher.exe: main.c ipodpatcher.c ipodio-win32.c parttypes.h $(BOOTSRC)
	$(CC) $(CFLAGS) -o ipodpatcher.exe main.c ipodpatcher.c ipodio-win32.c $(BOOTSRC)
	$(CROSS)strip ipodpatcher.exe

ipod2c: ipod2c.c
	$(NATIVECC) $(CFLAGS) -o ipod2c ipod2c.c

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
	rm -f ipodpatcher.exe ipodpatcher ipod2c *~ $(BOOTSRC) $(BOOT_H)
