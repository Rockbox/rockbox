#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

CC = $(PREFIX)gcc
CFLAGS=-Wall -W -g

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
EXT=.exe
CFLAGS+=-mno-cygwin
else ifeq ($(findstring mingw32msvc,$(PREFIX)),mingw32msvc)
EXT=.exe
else
EXT=
endif

all: xmlconv$(EXT) btcreate$(EXT) btsearch$(EXT)

../shared/utf8_aux.o: ../shared/utf8_aux.h
	$(CC) $(CFLAGS) -c -o $@ ../shared/utf8_aux.c

btcreate$(EXT): btcreate.c btree.c btree.h ../shared/utf8_aux.o
	$(CC) $(CFLAGS) -o $@ btcreate.c btree.c ../shared/utf8_aux.o

btsearch$(EXT): ../shared/btsearch.c ../shared/utf8_aux.o
	$(CC) $(CFLAGS) -DBTSEARCH_MAIN -o $@ ../shared/btsearch.c ../shared/utf8_aux.o

xmlconv$(EXT): xmlconv.c xmlconv.h xmlentities.h
	$(CC) $(CFLAGS) -o $@ -O3 xmlconv.c -lpcre -lz -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE

clean:
	rm -f btcreate$(EXT) btsearch$(EXT) xmlconv$(EXT) utf8_aux.o
