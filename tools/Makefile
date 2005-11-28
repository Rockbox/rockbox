#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
CFLAGS := -O -ansi -g
LDFLAGS := -g

TARGETS := scramble descramble iaudio sh2d bmp2rb rdf2binary convbdf	\
 generate_rocklatin mkboot ipod_fw uclpack

all: $(TARGETS)
	@echo "tools done"

scramble: scramble.o iriver.o
descramble: descramble.o iriver.o
iaudio: iaudio.o

scramble.o: scramble.c iriver.h
descramble.o: descramble.c iriver.h
iriver.o: iriver.c iriver.h
iaudio.o: iaudio.c

sh2d: sh2d.c

bmp2rb:	bmp2rb.c
	$(CC) -DAPPLICATION_NAME=\"$@\" -g $+ -o $@

rdf2binary:	rdf2binary.c
	$(CC) -g $+ -o $@

mkboot:	mkboot.c
	$(CC) -g $+ -o $@

ipod_fw:	ipod_fw.c
	$(CC) -g $+ -o $@

convbdf: convbdf.c
	$(CC) -g $+ -o $@

generate_rocklatin: generate_rocklatin.c ../firmware/drivers/lcd-player-charset.c 
	$(CC) -DHAVE_LCD_CHARCELLS -D__CONFIG_H__ -I../firmware/export $+ -o $@

uclpack:
	$(MAKE) -C ucl

clean:
	rm -f $(TARGETS) $(shell for f in $(TARGETS) ; do echo $$f.exe $$f.o $$f.obj ; done) *.ajf *~
	$(MAKE) -C ucl clean

