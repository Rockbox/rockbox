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

CLEANALL := scramble descramble iriver sh2d bmp2rb rdf2binary convbdf	\
 generate_rocklatin mkboot ipod_fw codepages uclpack mi4 gigabeat database

all:
	@echo "Run make in your build directory!"

scramble: scramble.o iriver.o mi4.o gigabeat.o
descramble: descramble.o iriver.o gigabeat.o

scramble.o: scramble.c iriver.h mi4.h gigabeat.h
descramble.o: descramble.c iriver.h gigabeat.h
iriver.o: iriver.c iriver.h
gigabeat.o: gigabeat.c gigabeat.h
mi4.o: mi4.c mi4.h

sh2d: sh2d.c

bmp2rb:	bmp2rb.c
	$(SILENT)$(CC) -DAPPLICATION_NAME=\"$@\" -g $+ -o $@

rdf2binary:	rdf2binary.c
	$(SILENT)$(CC) -g $+ -o $@

mkboot:	mkboot.c
	$(SILENT)$(CC) -g $+ -o $@

ipod_fw:	ipod_fw.c
	$(SILENT)$(CC) -g $+ -o $@

database: database.c ../apps/tagcache.c ../apps/metadata.c \
../firmware/id3.c ../firmware/common/unicode.c \
../firmware/common/crc32.c ../uisimulator/common/io.c \
../firmware/mp3data.c ../firmware/logf.c
	$(SILENT)$(CC) -g -I../firmware/export -iquote ../firmware/include \
-D__PCTOOL__ -DHAVE_TAGCACHE -DROCKBOX_HAS_LOGF -DSIMULATOR -ldl -I../apps $+ -o $@

convbdf: convbdf.c
	$(SILENT)$(CC) -g $+ -o $@

codepages: codepages.c codepage_tables.c
	$(SILENT)$(CC) -g $+ -o $@

generate_rocklatin: generate_rocklatin.c ../firmware/drivers/lcd-player-charset.c 
	$(SILENT)$(CC) -DHAVE_LCD_CHARCELLS -D__CONFIG_H__ -I../firmware/export $+ -o $@

uclpack:
	$(SILENT)$(MAKE) -C ucl

wavtrim: wavtrim.c
	$(SILENT)$(CC) -g $+ -o $@

voicefont: voicefont.c
	$(SILENT)$(CC) -g $+ -o $@

clean:
	@echo "Cleaning tools"
	$(SILENT)rm -f $(CLEANALL) $(shell for f in $(CLEANALL) ; do echo $$f.exe $$f.o $$f.obj ; done) *.ajf *~
	$(SILENT)$(MAKE) -C ucl clean

