all: ipodpatcher

ipodpatcher: ipodpatcher.c ipodio-posix.c parttypes.h
	gcc -Wall -o ipodpatcher ipodpatcher.c ipodio-posix.c

ipodpatcher.exe: ipodpatcher.c ipodio-win32.c parttypes.h
	i586-mingw32msvc-gcc -Wall -o ipodpatcher.exe ipodpatcher.c ipodio-win32.c
	i586-mingw32msvc-strip ipodpatcher.exe

clean:
	rm -f ipodpatcher.exe ipodpatcher *~
