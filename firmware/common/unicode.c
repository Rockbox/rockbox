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

#include <stdio.h>
#include "file.h"
#include "debug.h"
#include "rbunicode.h"
#include "config.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define CODEPAGE_DIR    ROCKBOX_DIR"/codepages"
static int default_codepage = 0;
static int loaded_cp_table = 0;

#ifdef HAVE_LCD_BITMAP

#define MAX_CP_TABLE_SIZE  32768
#define NUM_TABLES             5

static const char *filename[NUM_TABLES] =
{
    CODEPAGE_DIR"/iso.cp",
    CODEPAGE_DIR"/932.cp",  /* SJIS    */
    CODEPAGE_DIR"/936.cp",  /* GB2312  */
    CODEPAGE_DIR"/949.cp",  /* KSX1001 */
    CODEPAGE_DIR"/950.cp"   /* BIG5    */
};

static const char cp_2_table[NUM_CODEPAGES] =
{
    0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 4, 5, 0
};

static const char *name_codepages[NUM_CODEPAGES+1] =
{
    "ISO-8859-1",
    "ISO-8859-7",
    "ISO-8859-8",
    "CP1251",
    "ISO-8859-11",
    "CP1256",
    "ISO-8859-9",
    "ISO-8859-2",
    "CP1250",
    "SJIS",
    "GB-2312",
    "KSX-1001",
    "BIG5",
    "UTF-8",
    "unknown"
};

#else /* !HAVE_LCD_BITMAP, reduced support */

#define MAX_CP_TABLE_SIZE  640
#define NUM_TABLES           1

static const char *filename[NUM_TABLES] =
{
    CODEPAGE_DIR"/isomini.cp",
};

static const char cp_2_table[NUM_CODEPAGES] =
{
    0, 1, 1, 1, 1, 1, 0
};

static const char *name_codepages[NUM_CODEPAGES+1] =
{
    "ISO-8859-1",
    "ISO-8859-7",
    "CP1251",
    "ISO-8859-9",
    "ISO-8859-2",
    "CP1250",
    "UTF-8",
    "unknown"
};

#endif

static unsigned short codepage_table[MAX_CP_TABLE_SIZE];

static const unsigned char utf8comp[6] =
{
    0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
};

/* Load codepage file into memory */
static int load_cp_table(int cp)
{
    int i=0;
    int table = cp_2_table[cp];
    int file, tablesize;
    unsigned char tmp[2];

    if (table == 0 || table == loaded_cp_table)
        return 1;

    file = open(filename[table-1], O_RDONLY|O_BINARY);

    if (file < 0) {
        DEBUGF("Can't open codepage file: %s.cp\n", filename[table-1]);
        return 0;
    }

    tablesize = filesize(file) / 2;

    if (tablesize > MAX_CP_TABLE_SIZE) {
        DEBUGF("Invalid codepage file: %s.cp\n", filename[table-1]);
        close(file);
        return 0;
    }

    while (i < tablesize) {
        if (!read(file, tmp, 2)) {
            DEBUGF("Can't read from codepage file: %s.cp\n", 
                    filename[table-1]);
            loaded_cp_table = 0;
            return 0;
        }
        codepage_table[i++] = (tmp[1] << 8) | tmp[0];
    }

    loaded_cp_table = table;
    close(file);
    return 1;
}

/* Encode a UCS value as UTF-8 and return a pointer after this UTF-8 char. */
unsigned char* utf8encode(unsigned long ucs, unsigned char *utf8)
{
    int tail = 0;

    if (ucs > 0x7F)
        while (ucs >> (5*tail + 6))
            tail++;

    *utf8++ = (ucs >> (6*tail)) | utf8comp[tail];
    while (tail--)
        *utf8++ = ((ucs >> (6*tail)) & (MASK ^ 0xFF)) | COMP;

    return utf8;
}

/* Recode an iso encoded string to UTF-8 */
unsigned char* iso_decode(const unsigned char *iso, unsigned char *utf8,
                          int cp, int count)
{
    unsigned short ucs, tmp;

    if (cp == -1) /* use default codepage */
        cp = default_codepage;

    if (!load_cp_table(cp)) cp = 0;

    while (count--) {
        if (*iso < 128 || cp == UTF_8) /* Already UTF-8 */
            *utf8++ = *iso++;

        else {

            /* cp tells us which codepage to convert from */
            switch (cp) {
                case ISO_8859_7:  /* Greek */
                case WIN_1251:    /* Cyrillic */
                case ISO_8859_9:  /* Turkish */
                case ISO_8859_2:  /* Latin Extended */
                case WIN_1250:    /* Central European */
#ifdef HAVE_LCD_BITMAP
                case ISO_8859_8:  /* Hebrew */
                case ISO_8859_11: /* Thai */
                case WIN_1256:    /* Arabic */
#endif
                    tmp = ((cp-1)*128) + (*iso++ - 128);
                    ucs = codepage_table[tmp];
                    break;

#ifdef HAVE_LCD_BITMAP
                case SJIS: /* Japanese */
                    if (*iso > 0xA0 && *iso < 0xE0) {
                        tmp = *iso++ | (0xA100 - 0x8000);
                        ucs = codepage_table[tmp];
                        break;
                    }

                case GB_2312:  /* Simplified Chinese */
                case KSX_1001: /* Korean */
                case BIG_5:    /* Traditional Chinese */
                    if (count < 1 || !iso[1]) {
                        ucs = *iso++;
                        break;
                    }

                    /* we assume all cjk strings are written
                       in big endian order */
                    tmp = *iso++ << 8;
                    tmp |= *iso++;
                    tmp -= 0x8000;
                    ucs = codepage_table[tmp];
                    count--;
                    break;
#endif /* HAVE_LCD_BITMAP */

                default:
                    ucs = *iso++;
                    break;
            }

            if (ucs == 0) /* unknown char, use replacement char */
                ucs = 0xfffd;
            utf8 = utf8encode(ucs, utf8);
        }
    }
    return utf8;
}

/* Recode a UTF-16 string with little-endian byte ordering to UTF-8 */
unsigned char* utf16LEdecode(const unsigned char *utf16, unsigned char *utf8,
        int count)
{
    unsigned long ucs;

    while (count > 0) {
        /* Check for a surrogate pair */
        if (utf16[1] >= 0xD8 && utf16[1] < 0xE0) {
            ucs = 0x10000 + ((utf16[0] << 10) | ((utf16[1] - 0xD8) << 18)
                    | utf16[2] | ((utf16[3] - 0xDC) << 8));
            utf16 += 4;
            count -= 2;
        } else {
            ucs = (utf16[0] | (utf16[1] << 8));
            utf16 += 2;
            count -= 1;
        }
        utf8 = utf8encode(ucs, utf8);
    }
    return utf8;
}

/* Recode a UTF-16 string with big-endian byte ordering to UTF-8 */
unsigned char* utf16BEdecode(const unsigned char *utf16, unsigned char *utf8,
        int count)
{
    unsigned long ucs;

    while (count > 0) {
        if (*utf16 >= 0xD8 && *utf16 < 0xE0) { /* Check for a surrogate pair */
            ucs = 0x10000 + (((utf16[0] - 0xD8) << 18) | (utf16[1] << 10)
                    | ((utf16[2] - 0xDC) << 8) | utf16[3]);
            utf16 += 4;
            count -= 2;
        } else {
            ucs = (utf16[0] << 8) | utf16[1];
            utf16 += 2;
            count -= 1;
        }
        utf8 = utf8encode(ucs, utf8);
    }
    return utf8;
}

#if 0 /* currently unused */
/* Recode any UTF-16 string to UTF-8 */
unsigned char* utf16decode(const unsigned char *utf16, unsigned char *utf8,
        unsigned int count)
{
    unsigned long ucs;

    ucs = *(utf16++) << 8;
    ucs |= *(utf16++);

    if (ucs == 0xFEFF) /* Check for BOM */
        return utf16BEdecode(utf16, utf8, count-1);
    else if (ucs == 0xFFFE)
        return utf16LEdecode(utf16, utf8, count-1);
    else { /* ADDME: Should default be LE or BE? */
        utf16 -= 2;
        return utf16BEdecode(utf16, utf8, count);
    }
}
#endif

/* Return the number of UTF-8 chars in a string */
unsigned long utf8length(const unsigned char *utf8)
{
    unsigned long l = 0;

    while (*utf8 != 0)
        if ((*utf8++ & MASK) != COMP)
            l++;

    return l;
}

/* Decode 1 UTF-8 char and return a pointer to the next char. */
const unsigned char* utf8decode(const unsigned char *utf8, unsigned short *ucs)
{
    unsigned char c = *utf8++;
    unsigned long code;
    int tail = 0;

    if ((c <= 0x7f) || (c >= 0xc2)) {
        /* Start of new character. */
        if (c < 0x80) {        /* U-00000000 - U-0000007F, 1 byte */
            code = c;
        } else if (c < 0xe0) { /* U-00000080 - U-000007FF, 2 bytes */
            tail = 1;
            code = c & 0x1f;
        } else if (c < 0xf0) { /* U-00000800 - U-0000FFFF, 3 bytes */
            tail = 2;
            code = c & 0x0f;
        } else if (c < 0xf5) { /* U-00010000 - U-001FFFFF, 4 bytes */
            tail = 3;
            code = c & 0x07;
        } else {
            /* Invalid size. */
            code = 0xfffd;
        }

        while (tail-- && ((c = *utf8++) != 0)) {
            if ((c & 0xc0) == 0x80) {
                /* Valid continuation character. */
                code = (code << 6) | (c & 0x3f);

            } else {
                /* Invalid continuation char */
                code = 0xfffd;
                utf8--;
                break;
            }
        }
    } else {
        /* Invalid UTF-8 char */
        code = 0xfffd;
    }
    /* currently we don't support chars above U-FFFF */
    *ucs = (code < 0x10000) ? code : 0xfffd;
    return utf8;
}

void set_codepage(int cp)
{
    default_codepage = cp;
    return;
}

/* seek to a given char in a utf8 string and
   return its start position in the string */
int utf8seek(const unsigned char* utf8, int offset)
{
    int pos = 0;

    while (offset--) {
        pos++;
        while ((utf8[pos] & MASK) == COMP)
            pos++;
    }
    return pos;
}

const char* get_codepage_name(int cp)
{
    if (cp < 0 || cp>= NUM_CODEPAGES)
        return name_codepages[NUM_CODEPAGES];
    return name_codepages[cp];
}
