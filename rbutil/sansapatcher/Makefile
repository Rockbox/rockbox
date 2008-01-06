CFLAGS=-Wall -W -D_LARGEFILE64_SOURCE

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
OUTPUT=sansapatcher.exe
CROSS=
CFLAGS+=-mno-cygwin
else
ifeq ($(findstring MINGW,$(shell uname)),MINGW)
OUTPUT=sansapatcher.exe
CROSS
else
OUTPUT=sansapatcher
CROSS=i586-mingw32msvc-
endif
endif

NATIVECC = gcc
CC = $(CROSS)gcc

all: $(OUTPUT)

sansapatcher: main.c sansapatcher.c sansaio-posix.c parttypes.h bootimg_c200.c bootimg_e200.c
	gcc $(CFLAGS) -o sansapatcher main.c sansapatcher.c sansaio-posix.c bootimg_c200.c bootimg_e200.c
	strip sansapatcher

sansapatcher.exe: main.c sansapatcher.c sansaio-win32.c parttypes.h bootimg_c200.c bootimg_e200.c
	$(CC) $(CFLAGS) -o sansapatcher.exe main.c sansapatcher.c sansaio-win32.c bootimg_c200.c bootimg_e200.c
	$(CROSS)strip sansapatcher.exe

sansapatcher-mac: sansapatcher-i386 sansapatcher-ppc
	lipo -create sansapatcher-ppc sansapatcher-i386 -output sansapatcher-mac

sansapatcher-i386: main.c sansapatcher.c sansaio-posix.c parttypes.h bootimg_c200.c bootimg_e200.c
	gcc -isysroot /Developer/SDKs/MacOSX10.4u.sdk -o bin/i386/program -arch i386 $(CFLAGS) -o sansapatcher-i386 main.c sansapatcher.c sansaio-posix.c bootimg_c200.c bootimg_e200.c
	strip sansapatcher-i386

sansapatcher-ppc: main.c sansapatcher.c sansaio-posix.c parttypes.h bootimg_c200.c bootimg_e200.c
	gcc -arch ppc $(CFLAGS) -o sansapatcher-ppc main.c sansapatcher.c sansaio-posix.c bootimg_c200.c bootimg_e200.c
	strip sansapatcher-ppc

bin2c: bin2c.c
	$(NATIVECC) $(CFLAGS) -o bin2c bin2c.c

bootimg_c200.c: firmware.mi4 bin2c
	./bin2c firmware.mi4 bootimg_c200

bootimg_e200.c: PP5022.mi4 bin2c
	./bin2c PP5022.mi4 bootimg_e200

clean:
	rm -f sansapatcher.exe sansapatcher-mac sansapatcher-i386 sansapatcher-ppc sansapatcher bin2c bootimg_c200.c bootimg_c200.h bootimg_e200.c bootimg_e200.h *~
