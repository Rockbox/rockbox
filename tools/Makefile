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

CLEANALL := scramble descramble iaudio sh2d bmp2rb rdf2binary convbdf	\
 generate_rocklatin mkboot ipod_fw codepages uclpack

all:
	@echo "Run make in your build directory!"

scramble: scramble.o iriver.o
descramble: descramble.o iriver.o
iaudio: iaudio.o

scramble.o: scramble.c iriver.h
descramble.o: descramble.c iriver.h
iriver.o: iriver.c iriver.h
iaudio.o: iaudio.c

sh2d: sh2d.c

bmp2rb:	bmp2rb.c
	$(SILENT)$(CC) -DAPPLICATION_NAME=\"$@\" -g $+ -o $@

rdf2binary:	rdf2binary.c
	$(SILENT)$(CC) -g $+ -o $@

mkboot:	mkboot.c
	$(SILENT)$(CC) -g $+ -o $@

ipod_fw:	ipod_fw.c
	$(SILENT)$(CC) -g $+ -o $@

convbdf: convbdf.c
	$(SILENT)$(CC) -g $+ -o $@

codepages: codepages.c codepage_tables.c
	$(SILENT)$(CC) -g $+ -o $@

generate_rocklatin: generate_rocklatin.c ../firmware/drivers/lcd-player-charset.c 
	$(SILENT)$(CC) -DHAVE_LCD_CHARCELLS -D__CONFIG_H__ -I../firmware/export $+ -o $@

uclpack:
	$(SILENT)$(MAKE) -C ucl

clean:
	@echo "Cleaning tools"
	$(SILENT)rm -f $(CLEANALL) $(shell for f in $(CLEANALL) ; do echo $$f.exe $$f.o $$f.obj ; done) *.ajf *~
	$(SILENT)$(MAKE) -C ucl clean

