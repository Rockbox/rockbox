#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
CFLAGS := -O -s -ansi

TARGETS := scramble descramble sh2d bmp2rb convbdf

all: $(TARGETS)

scramble: scramble.c

descramble: descramble.c

sh2d: sh2d.c

bmp2rb:	bmp2rb.c
	$(CC) -DAPPLICATION_NAME=\"$@\" -g $+ -o $@

convbdf: convbdf.c
	$(CC) -g $+ -o $@

clean:
	rm -f $(TARGETS) $(shell for f in $(TARGETS) ; do echo $$f.exe $$f.o $$f.obj ; done) *.ajf *~

