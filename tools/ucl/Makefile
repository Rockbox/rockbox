#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
CFLAGS = -O2 -Iinclude

TARGET = ../uclpack

$(TARGET): uclpack.o src/libucl.a
	$(CC) uclpack.o src/libucl.a -o $(TARGET)

uclpack.o: uclpack.c

src/libucl.a:
	$(MAKE) -C src

clean:
	rm -f $(TARGET) uclpack.o
	$(MAKE) -C src clean