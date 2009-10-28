#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
TOOLSDIR=../../tools
CFLAGS := -O -g -W -Wall -Wshadow -pedantic -I$(TOOLSDIR)

all: mktccboot

telechips.o: $(TOOLSDIR)/telechips.c $(TOOLSDIR)/telechips.h
	$(SILENT)$(CC) $(CFLAGS) $(TOOLSDIR)/telechips.c -c -o $@

mktccboot: mktccboot.c telechips.o
	$(SILENT)$(CC) $(CFLAGS) $+ -o $@

clean:
	rm -f telechips.o mktccboot
