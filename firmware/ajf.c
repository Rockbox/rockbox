/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alex Gitelman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifdef SIMULATOR
#include <fcntl.h>
#endif
#include <file.h>
#include "ajf.h"
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include "debug.h"

static unsigned char font_buf[MAX_FONT_BUFLEN];

unsigned char* ajf_read_font(char* fname)
{
    int count;
#ifdef WIN32
    int fd = open(fname, O_RDONLY|O_BINARY);
#else
    int fd = open(fname, O_RDONLY);
#endif
    if (fd<0)
    {
#ifdef SIMULATOR
#ifdef WIN32
        DEBUGF("Failed opening font file: %d %s. ", _errno(), fname);
#else
        DEBUGF("Failed opening font file: %d %s. ", errno, fname);
#endif
#endif
        return NULL;
    }

    count = read(fd, font_buf, MAX_FONT_BUFLEN);
    if (count==MAX_FONT_BUFLEN) {
        DEBUGF("Font is larger than allocated %d bytes!\n",MAX_FONT_BUFLEN);
        return NULL;
    }
    close(fd);

    if (font_buf[0]!=MAGIC1 || font_buf[1]!=MAGIC2) {
        DEBUGF("Bad magic word in font");
        return NULL;
    }
    return font_buf;
}


unsigned char* ajf_get_charbuf(unsigned char c, unsigned char* font, 
                               int *w, int *h)
{
    int height = READ_SHORT(&font[HEIGHT_OFFSET]);
    int size = READ_SHORT(&font[SIZE_OFFSET]);
    int chars_offset = LOOKUP_MAP_OFFSET + size*3;
    int rows = (height-1)/8 + 1;
    int first_char = READ_SHORT(&font[FIRST_CHAR_OFFSET]); 
    int map_idx = LOOKUP_MAP_OFFSET + (c-first_char)*3;
    int byte_count = font[map_idx];
    int char_idx;

    *h = height;
    *w = byte_count/rows;

    map_idx++;
    char_idx = READ_SHORT(&font[map_idx]);
    return &font[chars_offset + char_idx];
}

void ajf_get_charsize(unsigned char c, unsigned char* font, 
                      int *width, int *height)
{
    int first_char = READ_SHORT(&font[FIRST_CHAR_OFFSET]); 
    int map_idx = LOOKUP_MAP_OFFSET + (c-first_char)*3;
    int rows = 1;
    *height = READ_SHORT(&font[HEIGHT_OFFSET]);
    rows = (*height-1)/8 + 1;
    *width = font[map_idx]/rows;
}

int ajf_get_fontheight(unsigned char* font)
{
    return READ_SHORT(&font[HEIGHT_OFFSET]);
}
