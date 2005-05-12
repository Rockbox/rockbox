#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

PREFIX=i586-mingw32msvc-

TARGET = fwpatcher.exe

WINDRES = $(PREFIX)windres
CC = $(PREFIX)gcc

CFLAGS = -DUNICODE -D_UNICODE -I. -Os -s -fomit-frame-pointer
LDFLAGS = -lmingw32 -mwindows -s

OBJS= resource.o iriver.o main.o md5.o

$(TARGET) : $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

resource.o: resource.rc bootloader.bin rockbox.ico
	$(WINDRES) -v $< $@

iriver.o: iriver.c iriver.h

main.o: main.c

md5.o: md5.c md5.h

