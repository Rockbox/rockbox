#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

PREFIX=i586-mingw32msvc-

TARGET = fwpatchernu.exe
TARGETU = fwpatcher.exe

WINDRES = $(PREFIX)windres
CC = $(PREFIX)gcc

UNICODE = -DUNICODE -D_UNICODE
CFLAGS = -I. -Os -s -fomit-frame-pointer
LDFLAGS = -lmingw32 -mwindows -s

OBJS= resource.o iriver.o main.o md5.o
OBJSU= resource.o iriveru.o mainu.o md5.o

all: $(TARGET) $(TARGETU)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(TARGETU): $(OBJSU)
	$(CC) $(LDFLAGS) $(OBJSU) -o $@

resource.o: resource.rc bootloader.bin rockbox.ico
	$(WINDRES) -v $< $@

iriveru.o: iriver.c iriver.h
	$(CC) $(CFLAGS) $(UNICODE) -c $< -o $@

iriver.o: iriver.c iriver.h

mainu.o: main.c
	$(CC) $(CFLAGS) $(UNICODE) -c $< -o $@

main.o: main.c

md5.o: md5.c md5.h

clean:
	rm -f $(OBJSU) $(OBJS) $(TARGETU) $(TARGET)
