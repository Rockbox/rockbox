#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
CFLAGS := -O -s -ansi

TARGETS := scramble descramble sh2d bdf2ajf

all: $(TARGETS)

scramble: scramble.c

descramble: descramble.c

sh2d: sh2d.c

bdf2ajf: bdf2ajf.c
	gcc -O -ansi $+ -o $@

clean:
	rm -f $(TARGETS) $(shell for f in $(TARGETS) ; do echo $$f.exe $$f.o $$f.obj ; done) *.ajf *~

