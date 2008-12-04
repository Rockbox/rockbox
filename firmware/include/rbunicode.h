/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2004,2005 by Marcoen Hirschberg
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
/*   Some conversion functions for handling UTF-8
 *
 *   I got all the info from:
 *   http://www.cl.cam.ac.uk/~mgk25/unicode.html#utf-8
 *   and
 *   http://en.wikipedia.org/wiki/Unicode
 */
#ifndef _RBUNICODE_H_
#define _RBUNICODE_H_
 
#ifndef __PCTOOL__
#include "config.h"
#endif

#define MASK   0xC0 /* 11000000 */
#define COMP   0x80 /* 10x      */

#ifdef HAVE_LCD_BITMAP
 
enum codepages {
    ISO_8859_1 = 0, ISO_8859_7, ISO_8859_8, WIN_1251,
    ISO_8859_11, WIN_1256, ISO_8859_9, ISO_8859_2, WIN_1250,
    SJIS, GB_2312, KSX_1001, BIG_5, UTF_8, NUM_CODEPAGES
};

#else /* !HAVE_LCD_BITMAP, reduced support */

enum codepages {
    ISO_8859_1 = 0, ISO_8859_7, WIN_1251, ISO_8859_9,
    ISO_8859_2, WIN_1250, UTF_8, NUM_CODEPAGES
};

#endif

/* Encode a UCS value as UTF-8 and return a pointer after this UTF-8 char. */
unsigned char* utf8encode(unsigned long ucs, unsigned char *utf8);
unsigned char* iso_decode(const unsigned char *latin1, unsigned char *utf8, int cp, int count);
unsigned char* utf16LEdecode(const unsigned char *utf16, unsigned char *utf8, int count);
unsigned char* utf16BEdecode(const unsigned char *utf16, unsigned char *utf8, int count);
unsigned long utf8length(const unsigned char *utf8);
const unsigned char* utf8decode(const unsigned char *utf8, unsigned short *ucs);
void set_codepage(int cp);
int utf8seek(const unsigned char* utf8, int offset);
const char* get_codepage_name(int cp);
#endif /* _RBUNICODE_H_ */
