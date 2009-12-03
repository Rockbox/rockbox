CFLAGS=-Wall -W -DWITH_BOOTOBJS -I../../../tools

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
OUTPUT=beastpatcher.exe
CROSS=
CFLAGS+=-mno-cygwin
else
ifeq ($(findstring MINGW,$(shell uname)),MINGW)
OUTPUT=beastpatcher.exe
CROSS=
else
OUTPUT=beastpatcher
CROSS=i586-mingw32msvc-
endif
endif

ifneq ($(findstring MINGW,$(shell uname)),MINGW)
LIBS = /usr/local/lib/libmtp.a /usr/local/lib/libusb.a
CFLAGS += $(shell printf \
	'\#include <libmtp.h>\nlibmtp version: LIBMTP_VERSION\n' | \
	gcc -E -P - -o - | grep -q '^libmtp version: 0\.2' && echo '-DOLDMTP')
CFLAGS += $(shell printf \
	'\#include <libmtp.h>\nlibmtp version: LIBMTP_VERSION\n' | \
	gcc -E -P - -o - | grep -q '^libmtp version: 0\.2\.[0-5]' && echo '-DREALLYOLDMTP')
endif
WINLIBS = ../MTP_DLL.dll

NATIVECC = gcc
CC = $(CROSS)gcc

all: $(OUTPUT)

SOURCES = beastpatcher.c bootimg.c mknkboot.c main.c
HEADERS = beastpatcher.h mtp_common.h bootimg.h mknkboot.h
OBJS    = beastpatcher.o bootimg.o mknkboot.o main.o mtp_libmtp.o
MTPSRCS_W32 = mtp_win32.c
MTPSRCS_MTP = mtp_libmtp.c

beastpatcher: $(SOURCES) $(HEADERS) $(MTPSRCS_MTP)
	gcc $(CFLAGS) -DBEASTPATCHER -o beastpatcher $(SOURCES) $(MTPSRCS_MTP) $(LIBS)
	strip beastpatcher

beastpatcher.exe: $(SOURCES) $(HEADERS) $(MTPSRCS_W32) $(WINLIBS)
	$(CC) $(CFLAGS) -DBEASTPATCHER -o beastpatcher.exe $(SOURCES) $(MTPSRCS_W32) $(WINLIBS)
	$(CROSS)strip beastpatcher.exe

beastpatcher-mac: beastpatcher-i386 beastpatcher-ppc
	lipo -create beastpatcher-ppc beastpatcher-i386 -output beastpatcher-mac

beastpatcher.dmg: beastpatcher-mac
	mkdir -p beastpatcher-dmg
	cp -p beastpatcher-mac beastpatcher-dmg
	hdiutil create -srcfolder beastpatcher-dmg beastpatcher.dmg

beastpatcher-i386:  $(SOURCES) $(HEADERS) $(MTPSRCS_MTP) usb.h libmtp.h libusb-i386.a libmtp-i386.a 
	gcc -c -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -arch i386 $(CFLAGS) -I. -DBEASTPATCHER $(SOURCES) $(MTPSRCS_MTP)
	gcc -framework iokit -framework coreservices -arch i386 -o beastpatcher-i386 $(OBJS) libusb-i386.a libmtp-i386.a -liconv
	strip beastpatcher-i386
	
beastpatcher-ppc: $(SOURCES) $(HEADERS) $(MTPSRCS_MTP) usb.h libmtp.h libusb-ppc.a libmtp-ppc.a 
	gcc -c -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -arch ppc $(CFLAGS) -I. -DBEASTPATCHER $(SOURCES) $(MTPSRCS_MTP)
	gcc -framework iokit -framework coreservices -arch ppc -o beastpatcher-ppc $(OBJS) libusb-ppc.a libmtp-ppc.a -liconv
	strip beastpatcher-ppc

bin2c: ../../../rbutil/tools/bin2c.c
	$(NATIVECC) $(CFLAGS) -o bin2c ../../../rbutil/tools/bin2c.c

bootimg.c: bootloader.bin bin2c
	./bin2c bootloader.bin bootimg

mknkboot: mknkboot.c
	$(SILENT)$(NATIVECC) $(CFLAGS) $+ -o $@
clean:
	rm -f beastpatcher.exe beastpatcher-mac beastpatcher-i386 beastpatcher-ppc beastpatcher bin2c bootimg.c bootimg.h mknkboot *~ *.o beastpatcher.dmg
	rm -rf beastpatcher-dmg
