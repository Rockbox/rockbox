/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Alexandre Bourget
 *
 * Plugin to transform a Rockbox produced m3u playlist into something
 * understandable by the picky original iRiver firmware.
 *
 * Based on sort.c by the Rockbox team.
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"

PLUGIN_HEADER

static struct plugin_api* rb;

int buf_size;
static char *filename;
static int readsize;
static char *stringbuffer;
static char crlf[2] = "\r\n";

int read_buffer(int offset)
{
    int fd;
    
    fd = rb->open(filename, O_RDONLY);
    if(fd < 0)
        return 10 * fd - 1;

    /* Fill the buffer from the file */
    rb->lseek(fd, offset, SEEK_SET);
    readsize = rb->read(fd, stringbuffer, buf_size);
    rb->close(fd);

    if(readsize < 0)
        return readsize * 10 - 2;

    if(readsize == buf_size)
        return buf_size; /* File too big */

    return 0;
}

static int write_file(void)
{
    char tmpfilename[MAX_PATH+1];
    int fd;
    int rc;
    char *buf_ptr;
    char *str_begin;

    /* Create a temporary file */
    
    rb->snprintf(tmpfilename, MAX_PATH+1, "%s.tmp", filename);
    
    fd = rb->creat(tmpfilename, 0);
    if(fd < 0)
        return 10 * fd - 1;

    /* Let's make sure it always writes CR/LF and not only LF */
    buf_ptr = stringbuffer;
    str_begin = stringbuffer;
    do {
	/* Transform slashes into backslashes */
        if(*buf_ptr == '/')
	    *buf_ptr = '\\';

	if((*buf_ptr == '\r') || (*buf_ptr == '\n')) {
	    /* We have no complete string ? It's only a leading \n or \r ? */
	    if (!str_begin)
		continue;
	    
	    /* Terminate string */
	    *buf_ptr = 0;

	    /* Write our new string */
	    rc = rb->write(fd, str_begin, rb->strlen(str_begin));
	    if(rc < 0) {
		rb->close(fd);
		return 10 * rc - 2;
	    }
	    /* Write CR/LF */
	    rc = rb->write(fd, crlf, 2);
	    if(rc < 0) {
		rb->close(fd);
		return 10 * rc - 3;
	    }

	    /* Reset until we get a new line */
	    str_begin = NULL;

	}
	else {
	    /* We start a new line here */
	    if (!str_begin)
		str_begin = buf_ptr;
	}

	/* Next char, until ... */
    } while(buf_ptr++ < stringbuffer + readsize);

    rb->close(fd);

    /* Remove the original file */
    rc = rb->remove(filename);
    if(rc < 0) {
        return 10 * rc - 4;
    }

    /* Replace the old file with the new */
    rc = rb->rename(tmpfilename, filename);
    if(rc < 0) {
        return 10 * rc - 5;
    }
    
    return 0;
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    char *buf;
    int rc;

    filename = (char *)parameter;

    rb = api;

    buf = rb->plugin_get_audio_buffer(&buf_size); /* start munching memory */

    stringbuffer = buf;

    rb->lcd_clear_display();
    rb->splash(0, true, "Converting...");
    
    rc = read_buffer(0);
    if(rc == 0) {
        rb->lcd_clear_display();
        rb->splash(0, true, "Writing...");
        rc = write_file();

        if(rc < 0) {
            rb->lcd_clear_display();
            rb->splash(HZ, true, "Can't write file: %d", rc);
        } else {
            rb->lcd_clear_display();
            rb->splash(HZ, true, "Done");
        }
    } else {
        if(rc < 0) {
            rb->lcd_clear_display();
            rb->splash(HZ, true, "Can't read file: %d", rc);
        } else {
            rb->lcd_clear_display();
            rb->splash(HZ, true, "The file is too big");
        }
    }
    
    return PLUGIN_OK;
}
