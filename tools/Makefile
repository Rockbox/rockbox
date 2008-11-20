#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
CFLAGS := -O -g -W -Wall -Wshadow -pedantic
LDFLAGS := -g

.PHONY: rbspeexenc uclpack

CLEANALL := scramble descramble iriver sh2d bmp2rb rdf2binary convbdf	\
 generate_rocklatin mkboot ipod_fw codepages uclpack mi4 gigabeat database \
 lngdump telechips gigabeats creative hmac-sha1 mktccboot mknkboot rbspeexenc mkzenboot

all: scramble descramble sh2d rdf2binary mkboot mktccboot mknkboot mkzenboot \
	convbdf codepages uclpack rbspeexenc voicefont

scramble: scramble.o iriver.o mi4.o gigabeat.o gigabeats.o telechips.o iaudio_bl_flash.o creative.o hmac-sha1.o
descramble: descramble.o iriver.o gigabeat.o
scramble.o: scramble.c iriver.h mi4.h gigabeat.h telechips.h iaudio_bl_flash.h creative.h

descramble.o: descramble.c iriver.h gigabeat.h
creative.o: creative.c creative.h
hmac-sha1.o: hmac-sha1.c hmac-sha1.h
iriver.o: iriver.c iriver.h
gigabeat.o: gigabeat.c gigabeat.h
gigabeats.o: gigabeats.c gigabeats.h
mi4.o: mi4.c mi4.h
telechips.o: telechips.c telechips.h
iaudio_bl_flash.o: iaudio_bl_flash.c iaudio_bl_flash.h
iaudio_bl_flash.c iaudio_bl_flash.h: iaudio_bl_flash.bmp bmp2rb
	$(SILENT)./bmp2rb -f 7 -h . $< >iaudio_bl_flash.c

sh2d: sh2d.c

bmp2rb:	bmp2rb.c
	$(SILENT)$(CC) -DAPPLICATION_NAME=\"$@\" $(CFLAGS) $+ -o $@

rdf2binary:	rdf2binary.c
	$(SILENT)$(CC) $(CFLAGS) $+ -o $@

mkboot:	mkboot.c
	$(SILENT)$(CC) $(CFLAGS) $+ -o $@

mktccboot: mktccboot.c telechips.o
	$(SILENT)$(CC) $(CFLAGS) $+ -o $@

mknkboot: mknkboot.c
	$(SILENT)$(CC) $(CFLAGS) $+ -o $@

mkzenboot.o: mkzenboot.c
	$(SILENT)$(CC) $(CFLAGS) -DSTANDALONE -c -o $@ $+

mkzenboot: mkzenboot.o hmac-sha1.o
	$(SILENT)$(CC) $(LDFLAGS) -lz $+ -o $@

mkzenboot.exe: mkzenboot.o hmac-sha1.o
	$(SILENT)$(CC) $(LDFLAGS) -lz $+ C:\MingW\lib\libz.a -o $@

lngdump: lngdump.c
	$(SILENT)$(CC) $(CFLAGS) $+ -o $@

ipod_fw:	ipod_fw.c
	$(SILENT)$(CC) $(CFLAGS) $+ -o $@

database: database.c ../apps/tagcache.c ../apps/metadata.c \
../firmware/id3.c ../firmware/common/unicode.c \
../firmware/common/crc32.c ../uisimulator/common/io.c \
../firmware/mp3data.c ../firmware/logf.c ../firmware/replaygain.c \
../firmware/common/structec.c
	$(SILENT)$(CC) $(CFLAGS) -I../firmware/export -iquote ../firmware/include \
-D__PCTOOL__ -DHAVE_TAGCACHE -DROCKBOX_HAS_LOGF -DSIMULATOR \
-DCONFIG_CODEC=1 -ldl -I../apps $+ -o $@

checkwps: checkwps.c ../apps/gui/wps_parser.c ../apps/gui/wps_debug.c ../firmware/common/ctype.c ../apps/misc.c ../apps/recorder/bmp.c
	$(SILENT)$(CC) $(CFLAGS) -I ../apps/gui -I../firmware/export \
-D__PCTOOL__ -DDEBUG -DROCKBOX_HAS_LOGF -DIPOD_COLOR -D ROCKBOX_DIR_LEN -D WPS_DIR=\".\" \
-I../apps -I../firmware/target/arm/ipod -I../firmware/include $+ -o $@

convbdf: convbdf.c
	$(SILENT)$(CC) $(CFLAGS) $+ -o $@

codepages: codepages.c codepage_tables.c
	$(SILENT)$(CC) $(CFLAGS) $+ -o $@

player_unifont: player_unifont.c ../firmware/drivers/lcd-charset-player.c
	$(SILENT)$(CC) -DARCHOS_PLAYER -D__PCTOOL__ -I../firmware/export $+ -o $@

uclpack:
	$(SILENT)$(MAKE) -C ucl

rbspeexenc:
	$(SILENT)$(MAKE) -C rbspeex

wavtrim: wavtrim.c
	$(SILENT)$(CC) $(CFLAGS) $+ -o $@

voicefont: voicefont.c
	$(SILENT)$(CC) $(CFLAGS) $+ -o $@

usb_benchmark: usb_benchmark.c
	$(SILENT)$(CC) $(CFLAGS) -lusb $+ -o $@

clean:
	@echo "Cleaning tools"
	$(SILENT)rm -f $(CLEANALL) $(shell for f in $(CLEANALL) ; do echo $$f.exe $$f.o $$f.obj ; done) *.ajf *~
	$(SILENT)$(MAKE) -C ucl clean
	$(SILENT)$(MAKE) -C rbspeex clean

