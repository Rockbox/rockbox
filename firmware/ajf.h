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
#ifndef __AJF__
#define __AJF__

/* defined here because they are used by tools/bdf2ajz */
#define MAGIC1 0xBD
#define MAGIC2 0xFC
#define MAX_FONT_BUFLEN 4096

#define CODE_PAGE_OFFSET 2 /* 1 byte TODO: unused now */
#define FONT_NAME_OFFSET 3
#define FONT_NAME_LEN 32
#define MAX_WIDTH_OFFSET (FONT_NAME_OFFSET + FONT_NAME_LEN)  /* 2 byte */
#define HEIGHT_OFFSET (MAX_WIDTH_OFFSET + 2) /* 2 byte */
#define ASCENT_OFFSET (HEIGHT_OFFSET+2)      /* 2 byte ascent (baseline) height*/
#define FIRST_CHAR_OFFSET (HEIGHT_OFFSET+2)  /* 2 bytes first character of font*/
#define SIZE_OFFSET (FIRST_CHAR_OFFSET+2)    /* 2 bytes size of font (# encodings)*/
#define LOOKUP_MAP_OFFSET SIZE_OFFSET+2      /* Map to lookup char positions */


#define WRITE_SHORT(s,buf) { (buf)[0] = (s & 0xff00) >> 8 ; \
                             (buf)[1] = s & 0x00ff; }
#define READ_SHORT(buf) (((buf)[0]<<8) + (buf)[1])

unsigned char* ajf_read_font(char* fname);
unsigned char* ajf_get_charbuf(unsigned char c, unsigned char* font, 
                               int *width, int *height);
void ajf_get_charsize(unsigned char c, unsigned char* font,
                      int *width, int *height);
int ajf_get_fontheight(unsigned char* font);

extern char _font_error_msg[];


#endif
