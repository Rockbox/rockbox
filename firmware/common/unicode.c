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
#include "pathfuncs.h"
#include "core_alloc.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef O_NOISODECODE
#define O_NOISODECODE 0
#endif

#define getle16(p) (p[0] | (p[1] >> 8))
#define getbe16(p) ((p[1] << 8) | p[0])

static int default_codepage = 0;
static int default_cp_handle = 0;
static int loaded_cp = 0;

#if !defined (__PCTOOL__) && (CONFIG_PLATFORM & PLATFORM_NATIVE)
/* Because file scanning uses the default CP table when matching entries,
   on-demand loading is not feasible; we also must use the filesystem lock */
#include "dir.h"
#include "file_internal.h"
#define cp_lock_enter() file_internal_lock_WRITER()
#define cp_lock_leave() file_internal_unlock_WRITER()

#else /* APPLICATION */

#include "mutex.h"

#ifdef __PCTOOL__
#define yield()
#endif

static struct mutex cp_mutex SHAREDBSS_ATTR;
#define cp_lock_enter() mutex_lock(&cp_mutex)
#define cp_lock_leave() mutex_unlock(&cp_mutex)
#endif /* !APPLICATION */

struct cp_info
{
    const char * const name;
    const char * const filename;
};

#ifdef HAVE_LCD_BITMAP

#define MAX_CP_TABLE_SIZE  32768
#define CP_ISO_FN    "iso.cp"
#define CP_932_FN    "932.cp"  /* SJIS    */
#define CP_936_FN    "936.cp"  /* GB2312  */
#define CP_949_FN    "949.cp"  /* KSX1001 */
#define CP_950_FN    "950.cp"  /* BIG5    */

static const struct cp_info cp_info[NUM_CODEPAGES+1] =
{
    [0 ... NUM_CODEPAGES] = { "unknown"    , NULL      },
    [ISO_8859_1]          = { "ISO-8859-1" , NULL      },
    [ISO_8859_7]          = { "ISO-8859-7" , CP_ISO_FN },
    [ISO_8859_8]          = { "ISO-8859-8" , CP_ISO_FN },
    [WIN_1251]            = { "CP1251"     , CP_ISO_FN },
    [ISO_8859_11]         = { "ISO-8859-11", CP_ISO_FN },
    [WIN_1256]            = { "CP1256"     , CP_ISO_FN },
    [ISO_8859_9]          = { "ISO-8859-9" , CP_ISO_FN },
    [ISO_8859_2]          = { "ISO-8859-2" , CP_ISO_FN },
    [WIN_1250]            = { "CP1250"     , CP_ISO_FN },
    [WIN_1252]            = { "CP1252"     , CP_ISO_FN },
    [SJIS]                = { "SJIS"       , CP_932_FN },
    [GB_2312]             = { "GB-2312"    , CP_936_FN },
    [KSX_1001]            = { "KSX-1001"   , CP_949_FN },
    [BIG_5]               = { "BIG5"       , CP_950_FN },
    [UTF_8]               = { "UTF-8"      , NULL      },
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
    int cp = default_codepage;
    if (cp < 0 || cp>= NUM_CODEPAGES)
        return name_codepages[NUM_CODEPAGES];
    return name_codepages[cp];
}
#endif /* defined(APPLICATION) && defined(__linux__) */

#else /* !HAVE_LCD_BITMAP, reduced support */

#define MAX_CP_TABLE_SIZE  768

#define CP_ISO_FN "isomini.cp"

static const struct cp_info cp_info[NUM_CODEPAGES+1] =
{
    [0 ... NUM_CODEPAGES] = { "unknown"    , NULL      },
    [ISO_8859_1]          = { "ISO-8859-1" , NULL      },
    [ISO_8859_7]          = { "ISO-8859-7" , CP_ISO_FN },
    [WIN_1251]            = { "CP1251"     , CP_ISO_FN },
    [ISO_8859_9]          = { "ISO-8859-9" , CP_ISO_FN },
    [ISO_8859_2]          = { "ISO-8859-2" , CP_ISO_FN },
    [WIN_1250]            = { "CP1250"     , CP_ISO_FN },
    [WIN_1252]            = { "CP1252"     , CP_ISO_FN },
    [UTF_8]               = { "UTF-8"      , NULL      },
};

#endif /* HAVE_LCD_BITMAP */

static inline bool same_cp_file(int cp1, int cp2)
{
    return cp_info[cp1].filename == cp_info[cp2].filename;
}

/* non-default codepage table buffer (cannot be bufalloced! playback itself
   may be making the load request) */
static unsigned short codepage_table[MAX_CP_TABLE_SIZE+1];

static const unsigned char utf8comp[6] =
{
    0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
};

static inline void cptable_tohw16(uint16_t *buf, unsigned int count)
{
#ifdef ROCKBOX_BIG_ENDIAN
    for (unsigned int i = 0; i < count; i++)
        buf[i] = letoh16(buf[i]);
#endif
    (void)buf; (void)count;
}

static int move_callback(int handle, void *current, void *new)
{
    /* we don't keep a pointer but we have to stop it sometimes */
    return handle != default_cp_handle ? BUFLIB_CB_CANNOT_MOVE : BUFLIB_CB_OK;
    (void)current; (void)new;
}

static int alloc_and_load_cp_table(int cp, void *buf)
{
    static struct buflib_callbacks ops =
        { .move_callback = move_callback };

    /* alloc and read only if there is an associated file */
    const char *filename = cp_info[cp].filename;
    if (!filename)
        return 0;

    char path[MAX_PATH];
    if (path_append(path, CODEPAGE_DIR, filename, sizeof (path))
        >= sizeof (path)) {
        return -1;
    }

    int fd = open(path, O_RDONLY|O_NOISODECODE);
    if (fd < 0)
        return -1;

    off_t size = filesize(fd);

    if (size >= (ssize_t)sizeof (uint16_t) && size <= MAX_CP_TABLE_SIZE*2 &&
        !(size % (ssize_t)sizeof (uint16_t))) {

        /* if the buffer is provided, use that but don't alloc */
        int handle = buf ? 0 : core_alloc_ex(filename, size, &ops);
        if (handle > 0)
            buf = core_get_data(handle);

        if (buf && read(fd, buf, size) == size) {
            close(fd);
            cptable_tohw16(buf, size / sizeof (uint16_t));
            return handle;
        }

        if (handle > 0)
            core_free(handle);
    }

    close(fd);
    return -1;
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
    /* just hold the lock for now to protect the loaded codepage if a load is
       in progress so two or more don't try to load a new table at the same
       time; will want concurrency here later for threads using the same one */
    cp_lock_enter();

    if (cp < 0 || cp >= NUM_CODEPAGES)
        cp = default_codepage;

    uint16_t *cptable = NULL;

    if (!same_cp_file(cp, default_codepage)) {
        /* default is wrong; check what's loaded */
        if (!same_cp_file(cp, loaded_cp)) {
            /* loaded one is wrong too */
            cptable = codepage_table;
            if (alloc_and_load_cp_table(cp, codepage_table) == 0) {
                loaded_cp = cp;
            } else {
                cp = 0; /* failed; do ISO-8859-1 */
                loaded_cp = 0; /* table might be clobberu now */
            }
        }
    } else if (default_cp_handle > 0) {
        cptable = core_get_data(default_cp_handle);
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
                    ucs = cptable[tmp];
                    break;

#ifdef HAVE_LCD_BITMAP
                case SJIS: /* Japanese */
                    if (*iso > 0xA0 && *iso < 0xE0) {
                        tmp = *iso++ | (0xA100 - 0x8000);
                        ucs = cptable[tmp];
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
                    ucs = cptable[tmp];
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

    cp_lock_leave();
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
        cp = NUM_CODEPAGES;

    /* load first then swap if load is successful, else just leave it; if
       handle is 0 then we just free the current one; this won't happen often
       thus we don't worry about reusing it and avoid any clobbering */

    int handle = -1;

    while (1) {
        cp_lock_enter();

        if (same_cp_file(cp, default_codepage)) {
            /* table not or no longer changing */
            default_codepage = cp;

            if (handle > 0)
                core_free(handle);

            cp_lock_leave();
            return;
        }

        if (handle >= 0)
            break;

        cp_lock_leave();

        if (handle < 0) {
            handle = alloc_and_load_cp_table(cp, NULL);
            if (handle < 0)
                return;
        }
    }

    if (default_cp_handle > 0)
        core_free(default_cp_handle);

    default_cp_handle = handle;
    default_codepage = cp;

    cp_lock_leave();
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

const char * get_codepage_name(int cp)
{
    if (cp < 0 || cp >= NUM_CODEPAGES)
        cp = NUM_CODEPAGES;

    return cp_info[cp].name;
}

#if !(CONFIG_PLATFORM & PLATFORM_NATIVE)
void unicode_init(void)
{
    mutex_init(&cp_mutex);
}
#endif
