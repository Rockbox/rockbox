
ifndef V
SILENT = @
endif

CFLAGS += -Wall -W -D_LARGEFILE64_SOURCE

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
CROSS=
CFLAGS+=-mno-cygwin
else
ifeq ($(findstring MINGW,$(shell uname)),MINGW)
CROSS=
else
CROSS=i586-mingw32msvc-
endif
endif

NATIVECC = gcc
CC = $(CROSS)gcc
WINDRES = $(CROSS)windres


all: bin2c

bin2c: bin2c.c
	@echo CC $<
	$(SILENT)$(NATIVECC) $(CFLAGS) -o bin2c bin2c.c

clean:
	$(RM) bin2c bin2c.exe
