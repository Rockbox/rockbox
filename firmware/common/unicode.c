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
#include "system.h"
#include "thread.h"
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

#define getle16(p) (p[0] | (p[1] << 8))
#define getbe16(p) ((p[0] << 8) | p[1])

#if !defined (__PCTOOL__) && (CONFIG_PLATFORM & PLATFORM_NATIVE)
/* Because file scanning uses the default CP table when matching entries,
   on-demand loading is not feasible; we also must use the filesystem lock */
#include "file_internal.h"
#else /* APPLICATION */
#ifdef __PCTOOL__
#define yield()
#define DEFAULT_CP_STATIC_ALLOC
#endif
#define open_noiso_internal open
#endif /* !APPLICATION */

#if 0 /* not needed just now (will probably end up a spinlock) */
#include "mutex.h"
static struct mutex cp_mutex SHAREDBSS_ATTR;
#define cp_lock_init()   mutex_init(&cp_mutex)
#define cp_lock_enter()  mutex_lock(&cp_mutex)
#define cp_lock_leave()  mutex_unlock(&cp_mutex)
#else
#define cp_lock_init()   do {} while (0)
#define cp_lock_enter()  asm volatile ("")
#define cp_lock_leave()  asm volatile ("")
#endif

enum cp_tid
{
    CP_TID_NONE = -1,
    CP_TID_ISO,
    CP_TID_932,
    CP_TID_936,
    CP_TID_949,
    CP_TID_950,
};

struct cp_info
{
    int8_t      tid;
    const char  *filename;
    const char  *name;
};

#define MAX_CP_TABLE_SIZE  32768

#define CPF_ISO "iso.cp"
#define CPF_932 "932.cp"  /* SJIS    */
#define CPF_936 "936.cp"  /* GB2312  */
#define CPF_949 "949.cp"  /* KSX1001 */
#define CPF_950 "950.cp"  /* BIG5    */

static const struct cp_info cp_info[NUM_CODEPAGES+1] =
{
    [0 ... NUM_CODEPAGES] = { CP_TID_NONE, NULL   , "unknown"     },
    [ISO_8859_1]          = { CP_TID_NONE, NULL   , "ISO-8859-1"  },
    [ISO_8859_7]          = { CP_TID_ISO , CPF_ISO, "ISO-8859-7"  },
    [ISO_8859_8]          = { CP_TID_ISO , CPF_ISO, "ISO-8859-8"  },
    [WIN_1251]            = { CP_TID_ISO , CPF_ISO, "CP1251"      },
    [ISO_8859_11]         = { CP_TID_ISO , CPF_ISO, "ISO-8859-11" },
    [WIN_1256]            = { CP_TID_ISO , CPF_ISO, "CP1256"      },
    [ISO_8859_9]          = { CP_TID_ISO , CPF_ISO, "ISO-8859-9"  },
    [ISO_8859_2]          = { CP_TID_ISO , CPF_ISO, "ISO-8859-2"  },
    [WIN_1250]            = { CP_TID_ISO , CPF_ISO, "CP1250"      },
    [WIN_1252]            = { CP_TID_ISO , CPF_ISO, "CP1252"      },
    [SJIS]                = { CP_TID_932 , CPF_932, "SJIS"        },
    [GB_2312]             = { CP_TID_936 , CPF_936, "GB-2312"     },
    [KSX_1001]            = { CP_TID_949 , CPF_949, "KSX-1001"    },
    [BIG_5]               = { CP_TID_950 , CPF_950, "BIG5"        },
    [UTF_8]               = { CP_TID_NONE, NULL   , "UTF-8"       },
};

static int default_cp = INIT_CODEPAGE;
static int default_cp_tid = CP_TID_NONE;
static int default_cp_handle = 0;
static int volatile default_cp_table_ref = 0;

static int loaded_cp_tid = CP_TID_NONE;
static int volatile cp_table_ref = 0;
#define CP_LOADING BIT_N(sizeof(int)*8-1) /* guard against multi loaders */

/* non-default codepage table buffer (cannot be bufalloced! playback itself
   may be making the load request) */
static unsigned short codepage_table[MAX_CP_TABLE_SIZE+1];

#if defined(APPLICATION) && defined(__linux__)
static const char * const name_codepages_linux[NUM_CODEPAGES+1] =
{
    [0 ... NUM_CODEPAGES] = "unknown",
    [ISO_8859_1]          = "iso8859-1",
    [ISO_8859_7]          = "iso8859-7",
    [ISO_8859_8]          = "iso8859-8",
    [WIN_1251]            = "cp1251",
    [ISO_8859_11]         = "iso8859-11",
    [WIN_1256]            = "cp1256",
    [ISO_8859_9]          = "iso8859-9",
    [ISO_8859_2]          = "iso8859-2",
    [WIN_1250]            = "cp1250",
    /* iso8859-15 is closest, linux doesnt have a codepage named cp1252 */
    [WIN_1252]            = "iso8859-15",
    [SJIS]                = "cp932",
    [GB_2312]             = "cp936",
    [KSX_1001]            = "cp949",
    [BIG_5]               = "cp950",
    [UTF_8]               = "utf8",
};

const char *get_current_codepage_name_linux(void)
{
    int cp = default_cp;
    if (cp < 0 || cp>= NUM_CODEPAGES)
        cp = NUM_CODEPAGES;
    return name_codepages_linux[cp];
}
#endif /* defined(APPLICATION) && defined(__linux__) */

#ifdef DEFAULT_CP_STATIC_ALLOC
static unsigned short default_cp_table_buf[MAX_CP_TABLE_SIZE+1];
#define cp_table_get_data(handle) \
    default_cp_table_buf
#define cp_table_free(handle) \
    do {} while (0)
#define cp_table_alloc(filename, size, opsp) \
    ({ (void)(opsp); 1; })
#else
#define cp_table_alloc(filename, size, opsp) \
    core_alloc_ex((filename), (size), (opsp))
#define cp_table_free(handle) \
    core_free(handle)
#define cp_table_get_data(handle) \
    core_get_data(handle)
#endif

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
    /* we don't keep a pointer but we have to stop it if this applies to a
       buffer not yet swapped-in since it will likely be in use in an I/O
       call */
    return (handle != default_cp_handle || default_cp_table_ref != 0) ?
                BUFLIB_CB_CANNOT_MOVE : BUFLIB_CB_OK;
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

    /* must be opened without a chance of reentering from FS code */
    int fd = open_noiso_internal(path, O_RDONLY);
    if (fd < 0)
        return -1;

    off_t size = filesize(fd);

    if (size > 0 && size <= MAX_CP_TABLE_SIZE*2 &&
        !(size % (off_t)sizeof (uint16_t))) {

        /* if the buffer is provided, use that but don't alloc */
        int handle = buf ? 0 : cp_table_alloc(filename, size, &ops);
        if (handle > 0)
            buf = cp_table_get_data(handle);

        if (buf && read(fd, buf, size) == size) {
            close(fd);
            cptable_tohw16(buf, size / sizeof (uint16_t));
            return handle;
        }

        if (handle > 0)
            cp_table_free(handle);
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
    uint16_t *table = NULL;

    cp_lock_enter();

    if (cp < 0 || cp >= NUM_CODEPAGES)
        cp = default_cp;

    int tid = cp_info[cp].tid;

    while (1) {
        if (tid == default_cp_tid) {
            /* use default table */
            if (default_cp_handle > 0) {
                table = cp_table_get_data(default_cp_handle);
                default_cp_table_ref++;
            }

            break;
        }

        bool load = false;

        if (tid == loaded_cp_tid) {
            /* use loaded table */
            if (!(cp_table_ref & CP_LOADING)) {
                if (tid != CP_TID_NONE) {
                    table = codepage_table;
                    cp_table_ref++;
                }

                break;
            }
        } else if (cp_table_ref == 0) {
            load = true;
            cp_table_ref |= CP_LOADING;
        }

        /* alloc and load must be done outside the lock */
        cp_lock_leave();

        if (!load) {
            yield();
        } else if (alloc_and_load_cp_table(cp, codepage_table) < 0) {
            cp = INIT_CODEPAGE; /* table may be clobbered now */
            tid = cp_info[cp].tid;
        }

        cp_lock_enter();

        if (load) {
            loaded_cp_tid = tid;
            cp_table_ref &= ~CP_LOADING;
        }
    }

    cp_lock_leave();

    while (count--) {
        unsigned short ucs, tmp;

        if (*iso < 128 || cp == UTF_8) /* Already UTF-8 */
            *utf8++ = *iso++;

        else {
            /* tid tells us which table to use and how */
            switch (tid) {
                case CP_TID_ISO: /* Greek */
                                 /* Hebrew */
                                 /* Cyrillic */
                                 /* Thai */
                                 /* Arabic */
                                 /* Turkish */
                                 /* Latin Extended */
                                 /* Central European */
                                 /* Western European */
                    tmp = ((cp-1)*128) + (*iso++ - 128);
                    ucs = table[tmp];
                    break;

                case CP_TID_932: /* Japanese */
                    if (*iso > 0xA0 && *iso < 0xE0) {
                        tmp = *iso++ | (0xA100 - 0x8000);
                        ucs = table[tmp];
                        break;
                    }

                case CP_TID_936: /* Simplified Chinese */
                case CP_TID_949: /* Korean */
                case CP_TID_950: /* Traditional Chinese */
                    if (count < 1 || !iso[1]) {
                        ucs = *iso++;
                        break;
                    }

                    /* we assume all cjk strings are written
                       in big endian order */
                    tmp = *iso++ << 8;
                    tmp |= *iso++;
                    tmp -= 0x8000;
                    ucs = table[tmp];
                    count--;
                    break;

                default:
                    ucs = *iso++;
                    break;
            }

            if (ucs == 0) /* unknown char, use replacement char */
                ucs = 0xfffd;
            utf8 = utf8encode(ucs, utf8);
        }
    }

    if (table) {
        cp_lock_enter();
        if (table == codepage_table) {
            cp_table_ref--;
        } else {
            default_cp_table_ref--;
        }
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
        cp = NUM_CODEPAGES;

    /* load first then swap if load is successful, else just leave it; if
       handle is 0 then we just free the current one; this won't happen often
       thus we don't worry about reusing it and consequently avoid possible
       clobbering of the existing one */

    int handle = -1;
    int tid = cp_info[cp].tid;

    while (1) {
        cp_lock_enter();

        if (default_cp_tid == tid)
            break;

        if (handle >= 0 && default_cp_table_ref == 0) {
            int hold = default_cp_handle;
            default_cp_handle = handle;
            handle = hold;
            default_cp_tid = tid;
            break;
        }

        /* alloc and load must be done outside the lock */
        cp_lock_leave();

        if (handle < 0 && (handle = alloc_and_load_cp_table(cp, NULL)) < 0)
            return; /* OOM; change nothing */

        yield();
    }

    default_cp = cp;
    cp_lock_leave();

    if (handle > 0)
        cp_table_free(handle);
}

int get_codepage(void)
{
    return default_cp;
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

#if 0 /* not needed just now */
void unicode_init(void)
{
    cp_lock_init();
}
#endif
