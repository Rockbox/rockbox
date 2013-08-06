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
#include "config.h"
#include "file.h"
#include "debug.h"
#include "rbunicode.h"
#include "rbpaths.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef O_NOISODECODE
#define O_NOISODECODE 0
#endif

#define getle16(p) (p[0] | (p[1] >> 8))
#define getbe16(p) ((p[1] << 8) | p[0])

static int default_codepage = 0;
static int loaded_cp_table = 0;
static int cp_table_refcount = 0;

#if !defined (__PCTOOL__) && (CONFIG_PLATFORM & PLATFORM_NATIVE)
/* This needs special handling for the sake of filesystem driver code to
   prevent infinite recursion into the codepage loading function and
   deadlocking since codepages are loaded on demand.

   FAT uses iso_decode when encountering short names during dir scanning
   and the codepage file may have to be loaded to service the request.
   Loading the file, of course, uses directory scanning to find it, which
   may have to do conversions to UTF-8, leading to another request to
   load the codepage file; ad infinitum.
   */
#include "dir.h"
#include "file_internal.h"
#define cp_lock_enter() file_internal_aux_lock()
#define cp_lock_leave() file_internal_aux_unlock()

#else /* APPLICATION */

#include "mutex.h"

#ifdef __PCTOOL__
#define yield()
#endif

static struct mutex cp_mutex SHAREDBSS_ATTR;
#define cp_lock_enter() mutex_lock(&cp_mutex)
#define cp_lock_leave() mutex_unlock(&cp_mutex)
static ssize_t file_open_read_and_close_internal(
    const char *path, int oflag, void *buf, size_t nbyte, off_t offset)
{
    ssize_t bytesread = -1;

    int fd = open(path, oflag & ~O_CREAT);

    if (fd >= 0) {
        bytesread = read(fd, buf, nbyte);
        close(fd);
    }

    return bytesread;
    (void)offset;
}
#endif /* !APPLICATION */

#ifdef HAVE_LCD_BITMAP

#define MAX_CP_TABLE_SIZE  32768
#define NUM_TABLES             5

static const char * const filename[NUM_TABLES] =
{
    CODEPAGE_DIR"/iso.cp",
    CODEPAGE_DIR"/932.cp",  /* SJIS    */
    CODEPAGE_DIR"/936.cp",  /* GB2312  */
    CODEPAGE_DIR"/949.cp",  /* KSX1001 */
    CODEPAGE_DIR"/950.cp"   /* BIG5    */
};

static const char cp_2_table[NUM_CODEPAGES] =
{
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 4, 5, 0
};

static const char * const name_codepages[NUM_CODEPAGES+1] =
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
    "CP1252",
    "SJIS",
    "GB-2312",
    "KSX-1001",
    "BIG5",
    "UTF-8",
    "unknown"
};

#if defined(APPLICATION) && defined(__linux__)
static const char * const name_codepages_linux[NUM_CODEPAGES+1] =
{
    /* "ISO-8859-1" */ "iso8859-1",
    /* "ISO-8859-7" */ "iso8859-7",
    /* "ISO-8859-8" */ "iso8859-8",
    /* "CP1251"     */ "cp1251",
    /* "ISO-8859-11"*/ "iso8859-11",
    /* "CP1256"     */ "cp1256",
    /* "ISO-8859-9" */ "iso8859-9",
    /* "ISO-8859-2" */ "iso8859-2",
    /* "CP1250"     */ "cp1250",
    /* "CP1252"     */ "iso8859-15", /* closest, linux doesnt have a codepage named cp1252 */
    /* "SJIS"       */ "cp932",
    /* "GB-2312"    */ "cp936",
    /* "KSX-1001"   */ "cp949",
    /* "BIG5"       */ "cp950",
    /* "UTF-8"      */ "utf8",
    /* "unknown"    */ "cp437"
};

const char *get_current_codepage_name_linux(void)
{
    if (default_codepage < 0 || default_codepage >= NUM_CODEPAGES)
        return name_codepages_linux[NUM_CODEPAGES];
    return name_codepages_linux[default_codepage];
}
#endif

#else /* !HAVE_LCD_BITMAP, reduced support */

#define MAX_CP_TABLE_SIZE  768
#define NUM_TABLES           1

static const char * const filename[NUM_TABLES] = {
    CODEPAGE_DIR"/isomini.cp"
};

static const char cp_2_table[NUM_CODEPAGES] =
{
    0, 1, 1, 1, 1, 1, 1, 0
};

static const char * const name_codepages[NUM_CODEPAGES+1] =
{
    "ISO-8859-1",
    "ISO-8859-7",
    "CP1251",
    "ISO-8859-9",
    "ISO-8859-2",
    "CP1250",
    "CP1252",
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
static bool load_cp_table(int table)
{
    bool retval = false;
    ssize_t filesize = file_open_read_and_close_internal(
                            filename[table - 1],
                            O_RDONLY|O_BINARY|O_NOISODECODE,
                            codepage_table, MAX_CP_TABLE_SIZE, 0);
    if (filesize < 0) {
        DEBUGF("Can't open or read codepage file (%d): %s.cp\n",
               (int)filesize, filename[table - 1]);
    } else if (filesize > 2 && filesize <= MAX_CP_TABLE_SIZE*2) {
#ifdef ROCKBOX_BIG_ENDIAN
        for (int i = 0; i < filesize / 2; i++)
            codepage_table[i] = letoh16(codepage_table[i]);
#endif /* ROCKBOX_BIG_ENDIAN */
        loaded_cp_table = table;
        retval = true;
    } else {
        loaded_cp_table = 0;
    }

    return retval;
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
    if (cp < 0 || cp >= NUM_CODEPAGES)
        cp = default_codepage;

    int table = cp_2_table[cp];
    bool cp_lock = table != 0;

    if (cp_lock) {
        cp_lock_enter();

        while (table != loaded_cp_table) {
            if (cp_table_refcount > 0) {
                cp_lock_leave();
                yield();
                cp_lock_enter();
            } else {
                cp_lock = load_cp_table(table);
                if (!cp_lock)
                    cp = 0;
                break;
            }
        }

        if (cp_lock)
            cp_table_refcount++;

        cp_lock_leave();
    }

    while (count--) {
        unsigned short ucs, tmp;

        if (*iso < 128 || cp == UTF_8) /* Already UTF-8 */
            *utf8++ = *iso++;

        else {

            /* cp tells us which codepage to convert from */
            switch (cp) {
                case ISO_8859_7:  /* Greek */
                case WIN_1252:    /* Western European */
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

    if (cp_lock) {
        cp_lock_enter();
        cp_table_refcount--;
        cp_lock_leave();
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
            ucs = getle16(utf16);
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
            ucs = getbe16(utf16);
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
    if (cp < 0 || cp >= NUM_CODEPAGES)
        cp = 0;

    default_codepage = cp;
}

int get_codepage(void)
{
    return default_codepage;
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

#if !(CONFIG_PLATFORM & PLATFORM_NATIVE)
void unicode_init(void)
{
    mutex_init(&cp_mutex);
}
#endif
