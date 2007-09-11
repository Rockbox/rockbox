CFLAGS=-Wall -W

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
OUTPUT=e200rpatcher.exe
CROSS=
CFLAGS+=-mno-cygwin
else
OUTPUT=e200rpatcher
CROSS=i586-mingw32msvc-
endif

LIBS = -lusb

NATIVECC = gcc
CC = $(CROSS)gcc

all: $(OUTPUT)

e200rpatcher: e200rpatcher.c bootimg.c
	gcc $(CFLAGS) $(LIBS) -o e200rpatcher e200rpatcher.c bootimg.c
	strip e200rpatcher

e200rpatcher.exe:  e200rpatcher.c bootimg.c
	$(CC) $(CFLAGS) $(LIBS) -o e200rpatcher.exe e200rpatcher.c bootimg.c
	$(CROSS)strip e200rpatcher.exe

e200rpatcher-mac: e200rpatcher-i386 e200rpatcher-ppc
	lipo -create e200rpatcher-ppc e200rpatcher-i386 -output e200rpatcher-mac
e200rpatcher-i386:  e200rpatcher.c  bootimg.c
	gcc -isysroot /Developer/SDKs/MacOSX10.4u.sdk -o bin/i386/program -arch i386 $(CFLAGS) $(LIBS) -o e200rpatcher-i386 e200rpatcher.c bootimg.c
	strip e200rpatcher-i386

e200rpatcher-ppc: e200rpatcher.c bootimg.c
	gcc -arch ppc $(CFLAGS) $(LIBS) -o e200rpatcher-ppc e200rpatcher.c bootimg.c
	strip e200rpatcher-ppc

bin2c: ../sansapatcher/bin2c.c
	$(NATIVECC) $(CFLAGS) -o bin2c ../sansapatcher/bin2c.c

bootimg.c: bootloader.bin bin2c
	./bin2c bootloader.bin bootimg

clean:
	rm -f e200rpatcher.exe e200rpatcher-mac e200rpatcher-i386 e200rpatcher-ppc e200rpatcher bin2c bootimg.c bootimg.h *~
