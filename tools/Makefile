#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
CFLAGS := -O -ansi

TARGETS := scramble descramble sh2d bmp2rb convbdf generate_rocklatin

all: $(TARGETS)
	@echo "tools done"

scramble: scramble.o iriver.o
descramble: descramble.o iriver.o

scramble.o: scramble.c iriver.h
descramble.o: descramble.c iriver.h
iriver.o: iriver.c iriver.h

sh2d: sh2d.c

bmp2rb:	bmp2rb.c
	$(CC) -DAPPLICATION_NAME=\"$@\" -g $+ -o $@

convbdf: convbdf.c
	$(CC) -g $+ -o $@

generate_rocklatin: generate_rocklatin.c ../firmware/drivers/lcd-player-charset.c 
	$(CC) -DHAVE_LCD_CHARCELLS -I../firmware/export -g $+ -o $@

clean:
	rm -f $(TARGETS) $(shell for f in $(TARGETS) ; do echo $$f.exe $$f.o $$f.obj ; done) *.ajf *~

