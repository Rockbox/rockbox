#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

CFLAGS := -O3 -s -ansi

TARGETS := scramble descramble sh2d \
	scramble.static.bz2 descramble.static.bz2 sh2d.static.bz2

all: $(TARGETS)

scramble: scramble.c

descramble: descramble.c

sh2d: sh2d.c

clean:
	rm -f $(TARGETS) *~

descramble.static.bz2: descramble.c
	$(CC) -static $(CFLAGS) -o descramble.static $<
	bzip2 -f descramble.static
	chmod a+r descramble.static.bz2

scramble.static.bz2: scramble.c
	$(CC) -static $(CFLAGS) -o scramble.static $<
	bzip2 -f scramble.static
	chmod a+r scramble.static.bz2

sh2d.static.bz2: sh2d.c
	$(CC) -static $(CFLAGS) -o sh2d.static $<
	bzip2 -f sh2d.static
	chmod a+r sh2d.static.bz2
