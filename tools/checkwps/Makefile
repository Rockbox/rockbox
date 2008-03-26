#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 16822 2008-03-26 18:18:22Z dave $
#

all: checkwps

checkwps: checkwps.c ../../apps/gui/wps_parser.c ../../apps/gui/wps_debug.c ../../firmware/common/ctype.c ../../apps/misc.c ../../apps/recorder/bmp.c
	$(SILENT)$(CC) -g -I ../../apps/gui -I../../firmware/export \
-D__PCTOOL__ -DDEBUG -DROCKBOX_HAS_LOGF -DIPOD_COLOR -D ROCKBOX_DIR_LEN -D WPS_DIR=\".\" \
-I../../apps -I../../firmware/target/arm/ipod -I../../firmware/include $+ -o $@

clean:
	rm -f cleanwps
