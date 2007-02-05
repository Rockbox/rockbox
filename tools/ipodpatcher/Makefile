CFLAGS=-Wall

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
OUTPUT=ipodpatcher.exe
CROSS=
CFLAGS+=-mno-cygwin
else
OUTPUT=ipodpatcher
CROSS=i586-mingw32msvc-
endif

all: $(OUTPUT)

ipodpatcher: ipodpatcher.c ipodio-posix.c parttypes.h
	gcc $(CFLAGS) -o ipodpatcher ipodpatcher.c ipodio-posix.c
	strip ipodpatcher

ipodpatcher.exe: ipodpatcher.c ipodio-win32.c parttypes.h
	$(CROSS)gcc $(CFLAGS) -o ipodpatcher.exe ipodpatcher.c ipodio-win32.c
	$(CROSS)strip ipodpatcher.exe

clean:
	rm -f ipodpatcher.exe ipodpatcher *~
