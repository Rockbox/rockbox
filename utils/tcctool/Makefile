# This contains the file to upload
ROMFILE=player.rom

CFLAGS=-Wall -W -I/usr/local/include

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
OUTPUT=tcctool.exe
CROSS=
CFLAGS+=-mno-cygwin
else
OUTPUT=tcctool
CROSS=i586-mingw32msvc-
endif

LIBS = -lusb
WINLIBS = -I libusb-win32-device-bin-0.1.12.1/include libusb-win32-device-bin-0.1.12.1/lib/dynamic/libusb_dyn.c 

NATIVECC = gcc
CC = $(CROSS)gcc

all: $(OUTPUT)

tcctool: tcctool.c
	gcc $(CFLAGS) $(LIBS) -o tcctool tcctool.c
	strip tcctool

tcctool.exe:  tcctool.c
	$(CC) $(CFLAGS) $(WINLIBS) -o tcctool.exe tcctool.c
	$(CROSS)strip tcctool.exe

tcctool-mac: tcctool-i386 tcctool-ppc
	lipo -create tcctool-ppc tcctool-i386 -output tcctool-mac

tcctool-i386:  tcctool.c usb.h libusb-i386.a
	gcc -isysroot /Developer/SDKs/MacOSX10.4u.sdk -framework iokit -framework coreservices -arch i386 $(CFLAGS) -o tcctool-i386 tcctool.c -I. libusb-i386.a
	strip tcctool-i386

tcctool-ppc: tcctool.c usb.h libusb-ppc.a
	gcc -isysroot /Developer/SDKs/MacOSX10.4u.sdk -framework iokit -framework coreservices -arch ppc $(CFLAGS) -o tcctool-ppc tcctool.c -I. libusb-ppc.a
	strip tcctool-ppc

clean:
	rm -f tcctool.exe tcctool-mac tcctool-i386 tcctool-ppc tcctool *~
