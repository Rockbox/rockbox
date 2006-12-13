all: ipodpatcher.exe

ipodpatcher.exe: ipodpatcher.c
	i586-mingw32msvc-gcc -Wall -o ipodpatcher.exe ipodpatcher.c
	i586-mingw32msvc-strip ipodpatcher.exe

clean:
	rm -f ipodpatcher.exe *~
