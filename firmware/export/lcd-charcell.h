/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Jens Arnold
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
             
#include "config.h"

/* target dependent - to be adjusted for other charcell targets */
#define HW_PATTERN_SIZE 7  /* number of bytes per pattern */
#define MAX_HW_PATTERNS 8  /* max. number of user-definable hw patterns */

struct cursor_info {
    unsigned char hw_char;
    unsigned char subst_char;
    bool enabled;
    bool visible;
    int x;
    int y;
    int divider;
    int downcount;
};

/* map unicode characters to hardware or extended lcd characters */
struct xchar_info {
    unsigned short ucs;
    unsigned short glyph;
    /* 0x0000..0x7fff: fixed extended characters
     * 0x8000..0xffff: variable extended characters
     * Dontcare if priority == 0 */
    unsigned char priority;
    unsigned char hw_char;    /* direct or substitute */
};

/* track usage of user-definable characters */
struct pattern_info {
    short count;
    unsigned short glyph;
    unsigned char priority;
    unsigned char pattern[HW_PATTERN_SIZE];
};

extern int lcd_pattern_count;  /* actual number of user-definable hw patterns */

extern unsigned char lcd_charbuffer[LCD_HEIGHT][LCD_WIDTH];
extern struct pattern_info lcd_patterns[MAX_HW_PATTERNS];
extern struct cursor_info lcd_cursor;

extern const struct xchar_info *xchar_info;
extern int xchar_info_size;      /* number of entries */
extern const unsigned char xfont_fixed[][HW_PATTERN_SIZE];

void lcd_charset_init(void);

