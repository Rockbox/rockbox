/* Copyright (c) 2007, Christophe Fergeau  <teuf@gnome.org>
 * Part of the libgpod project.
 * 
 * URL: http://www.gtkpod.org/
 * URL: http://gtkpod.sourceforge.net/
 *
 * The code contained in this file is free software; you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either version
 * 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this code; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * iTunes and iPod are trademarks of Apple
 *
 * This product is not supported/written/published by Apple!
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <scsi/sg_cmds.h>
#include <endian.h>
#include <time.h>

static int do_sg_write_buffer (const char *device, void *buffer, size_t len)
{
    int fd;
    size_t i;

    fd = open (device, O_RDWR);
    if (fd < 0) {
	printf ("Couldn't open device %s\n", device);
	return -1;
    }

    printf ("    Data Payload: ");
    for (i = 0; i < len; i++) {
	printf ("%02x ", *((uint8_t *)buffer+i));
    }
    printf ("\n");

    if (sg_ll_write_buffer (fd, 1, 0, 0x0c0000, buffer, len, 1, 1) != 0) {
	close(fd);
	return -2;
    }
    close(fd);

    return 0;
}

int sync_time (const char *device, struct tm *tm)
{
    struct iPodTime {
	uint16_t year;
	uint16_t days;
	uint8_t timezone;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t dst;
	uint8_t padding[3];
    } __attribute__((__packed__));
    struct iPodTime ipod_time;

    if (tm == NULL) {
	time_t current_time;
	current_time = time (NULL);
	tm = localtime (&current_time);
    }

    ipod_time.year = htobe16 (1900+tm->tm_year);
    ipod_time.days = htobe16 (tm->tm_yday);
    /* the timezone value is the shift east of UTC in 15 min chunks
     * so eg. UTC+2 is 2*4 = 8
     */
    ipod_time.timezone = tm->tm_gmtoff / 900;
    ipod_time.hour = tm->tm_hour;
    ipod_time.minute = tm->tm_min;
    ipod_time.second = tm->tm_sec;
    if (tm->tm_isdst) {
	ipod_time.dst = 1;
    } else {
	ipod_time.dst = 0;
    }
    memset (ipod_time.padding, 0, sizeof (ipod_time.padding));

    return do_sg_write_buffer (device, &ipod_time, sizeof (ipod_time));
}
