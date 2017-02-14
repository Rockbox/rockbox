/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 * Copyright (C) 2014 by Michael Sevakis
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
#define __FILESYS_STRUCTS_DRV
#include "config.h"
#include "system.h"
#include "sys/types.h"
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "fs_attr.h"
#include "pathfuncs.h"
#include "file_internal.h"
#include "dir.h"
#include "storage.h"
#include "timefuncs.h"
#include "rbunicode.h"
#include "debug.h"
#include "panic.h"
#include "string-extra.h"

/* cache hints on what's more important to keep */
#define SECPRIO_DATA    1
#define SECPRIO_FAT     3
#define SECPRIO_DIRENT  2

/* fake compare-and-swaps for now (to extend cluster chain) */
static inline uint16_t __fat_cas16(uint16_t *addr,
                                   uint16_t cmpval,
                                   uint16_t swpval)
{
    *addr = swpval;
    return cmpval;
}

static inline uint32_t __fat_cas32(uint32_t *addr,
                                   uint32_t cmpval,
                                   uint32_t swpval)
{
    *addr = swpval;
    return cmpval;
}

/* just compiler barriers for now (but type-checked) */
#define __fsinfo_lock_init(_bpb) \
    do { struct bpb *__bpb = (_bpb); (void)__bpb; \
    } while (0)

#define __fsinfo_lock(_bpb) \
    do { struct bpb *__bpb = (_bpb); (void)__bpb; \
         asm volatile("" : : : "memory");         \
    } while (0)

#define __fsinfo_unlock(_bpb) \
    do { struct bpb *__bpb = (_bpb); (void)__bpb; \
         asm volatile("" : : : "memory");         \
    } while (0)

static inline void fsinfo_freecount_dec(struct bpb *bpb,
                                        long nextfree)
{
    __fsinfo_lock(bpb);

    bpb->fsinfo.nextfree = nextfree;
    if (bpb->fsinfo.freecount > 0)
        bpb->fsinfo.freecount--;

    bpb->fsinfo.dirty = true;

    __fsinfo_unlock(bpb);
}

static inline void fsinfo_freecount_inc(struct bpb *bpb)
{
    __fsinfo_lock(bpb);

    bpb->fsinfo.freecount++;
    bpb->fsinfo.dirty = true;

    __fsinfo_unlock(bpb);
}

#define BYTES2INT32(array, pos) \
    (((uint32_t)array[pos+0] <<  0) | \
     ((uint32_t)array[pos+1] <<  8) | \
     ((uint32_t)array[pos+2] << 16) | \
     ((uint32_t)array[pos+3] << 24))

#define INT322BYTES(array, pos, val) \
    ((array[pos+0] = (uint32_t)(val) >>  0), \
     (array[pos+1] = (uint32_t)(val) >>  8), \
     (array[pos+2] = (uint32_t)(val) >> 16), \
     (array[pos+3] = (uint32_t)(val) >> 24))

#define BYTES2INT16(array, pos) \
    (((uint32_t)array[pos+0] << 0) | \
     ((uint32_t)array[pos+1] << 8))

#define INT162BYTES(array, pos, val) \
    ((array[pos+0] = (uint16_t)(val) >> 0), \
     (array[pos+1] = (uint16_t)(val) >> 8))

#define FATTYPE_FAT12       0
#define FATTYPE_FAT16       1
#define FATTYPE_FAT32       2

/* BPB offsets; generic */
#define BS_JMPBOOT          0
#define BS_OEMNAME          3
#define BPB_BYTSPERSEC      11
#define BPB_SECPERCLUS      13
#define BPB_RSVDSECCNT      14
#define BPB_NUMFATS         16
#define BPB_ROOTENTCNT      17
#define BPB_TOTSEC16        19
#define BPB_MEDIA           21
#define BPB_FATSZ16         22
#define BPB_SECPERTRK       24
#define BPB_NUMHEADS        26
#define BPB_HIDDSEC         28
#define BPB_TOTSEC32        32

/* fat12/16 */
#define BS_DRVNUM           36
#define BS_RESERVED1        37
#define BS_BOOTSIG          38
#define BS_VOLID            39
#define BS_VOLLAB           43
#define BS_FILSYSTYPE       54

/* fat32 */
#define BPB_FATSZ32         36
#define BPB_EXTFLAGS        40
#define BPB_FSVER           42
#define BPB_ROOTCLUS        44
#define BPB_FSINFO          48
#define BPB_BKBOOTSEC       50
#define BS_32_DRVNUM        64
#define BS_32_BOOTSIG       66
#define BS_32_VOLID         67
#define BS_32_VOLLAB        71
#define BS_32_FILSYSTYPE    82

#define BPB_LAST_WORD       510

/* used to collect and verify BPB info */
struct fatbpb
{
    /* generic */
    uint16_t bpb_bytspersec;
    uint8_t  bpb_secperclus;
    uint16_t bpb_rsvdseccnt;
    uint8_t  bpb_numfats;
    uint16_t bpb_rootentcnt;
    uint16_t bpb_totsec16;
    uint8_t  bpb_media;
    uint16_t bpb_fatsz16;
    uint32_t bpb_totsec32;

    /* fat12/16 */

    /* fat32 */
    uint32_t bpb_fatsz32;
    uint16_t bpb_extflags;

    /* all */
    uint16_t bpb_lastword;
    bool     bpb_isfat16;
};

/* Short and long name directory entry template */
union raw_dirent
{
    struct /* short entry */
    {
        uint8_t  name[8+3];         /*  0 */
        uint8_t  attr;              /* 11 */
        uint8_t  ntres;             /* 12 */
        uint8_t  crttimetenth;      /* 13 */
        uint16_t crttime;           /* 14 */
        uint16_t crtdate;           /* 16 */
        uint16_t lstaccdate;        /* 18 */
        uint16_t fstclushi;         /* 20 */
        uint16_t wrttime;           /* 22 */
        uint16_t wrtdate;           /* 24 */
        uint16_t fstcluslo;         /* 26 */
        uint32_t filesize;          /* 28 */
                                    /* 32 */
    };
    struct /* long entry */
    {
        uint8_t  ldir_ord;          /*  0 */
        uint8_t  ldir_name1[10];    /*  1 */
        uint8_t  ldir_attr;         /* 11 */
        uint8_t  ldir_type;         /* 12 */
        uint8_t  ldir_chksum;       /* 13 */
        uint8_t  ldir_name2[12];    /* 14 */
        uint16_t ldir_fstcluslo;    /* 26 */
        uint8_t  ldir_name3[4];     /* 28 */
                                    /* 32 */
    };
    struct /* raw byte array */
    {
        uint8_t  data[32];          /*  0 */
                                    /* 32 */
    };
};

/* Number of bytes reserved for a file name (including the trailing \0).
 * Since names are stored in the entry as UTF-8, we won't be ble to
 * store all names allowed by FAT. In FAT, a name can have max 255
 * characters (not bytes!). The worst case UTF-8 conversion is where
 * each two-byte UTF-16 character expands to a 3-byte UTF-8 character,
 * requiring 765 bytes for the final name.
 *
 * U+000000-U+00007F: 2 bytes => 1 byte  (-1 / char)
 * U+000080-U+0007FF: 2 bytes => 2 bytes (+0 / char)
 * U+000800-U+00FFFF: 2 bytes => 3 bytes (+1 / char)
 * U+010000-U+10FFFF: 4 bytes => 4 bytes (+0 / char)
 */
#define FAT_UTF8_NAME_MAX       255 /* our limit */
#define MAX_DIRENTRIES        65536
#define MAX_DIRECTORY_SIZE (MAX_DIRENTRIES*32) /* 2MB max size */

/* at most 20 LFN entries */
#define FATLONG_MAX_ORDER           20
#define FATLONG_NAME_CHARS          13
#define FATLONG_ORD_F_LAST          0x40

/* attributes */
#define ATTR_LONG_NAME          (ATTR_READ_ONLY | ATTR_HIDDEN | \
                                 ATTR_SYSTEM | ATTR_VOLUME_ID)
#define ATTR_LONG_NAME_MASK     (ATTR_READ_ONLY | ATTR_HIDDEN | \
                                 ATTR_SYSTEM | ATTR_VOLUME_ID | \
                                 ATTR_DIRECTORY | ATTR_ARCHIVE )

#define IS_LDIR_ATTR(attr) \
    (((attr) & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME)

#define IS_VOL_ID_ATTR(attr) \
    (((attr) & (ATTR_VOLUME_ID | ATTR_DIRECTORY)) == ATTR_VOLUME_ID)

/* NTRES flags */
#define FAT_NTRES_LC_NAME           0x08
#define FAT_NTRES_LC_EXT            0x10

#define CLUSTERS_PER_FAT32_SECTOR   (SECTOR_SIZE / 4)
#define CLUSTERS_PER_FAT16_SECTOR   (SECTOR_SIZE / 2)
#define DIR_ENTRIES_PER_SECTOR      (SECTOR_SIZE / DIR_ENTRY_SIZE)
#define DIR_ENTRY_SIZE              32
#define FAT32_BAD_MARK              0x0ffffff7
#define FAT32_EOF_MARK              0x0ffffff8
#define FAT16_EOF_MARK              0xfff8
#define FAT16_BAD_MARK              0xfff7
#define FAT_BAD_MARK                0x0ffffff7
#define FAT_EOF_MARK                0x0ffffff8

/* fsinfo offsets */
#define FSINFO_FREECOUNT 488
#define FSINFO_NEXTFREE  492

#ifdef HAVE_FAT16SUPPORT
#define BPB_ISFAT16(bpb) \
    ((bpb)->rootclus < 0)
#define SET_FAT_OPS(bpb, fatops) \
    ((bpb)->ops = (fatops))
#define IS_FAT16_ROOTCLUS(bpb, cluster) \
    (BPB_ISFAT16(bpb) && (cluster) < 0)
#define get_next_cluster(bpb, ...) \
    ((bpb)->ops->ops_get_next_cluster((bpb), __VA_ARGS__))
#define alloc_cluster(bpb, ...) \
    ((bpb)->ops->ops_alloc_cluster((bpb), __VA_ARGS__))
#define update_fat_entry(bpb, ...) \
    ((bpb)->ops->ops_update_fat_entry((bpb), __VA_ARGS__))
#else  /* !HAVE_FAT16SUPPORT */
#define BPB_ISFAT16(bpb) \
    (false)
#define SET_FAT_OPS(bpb, fatops) \
    do {} while(0)
#define IS_FAT16_ROOTCLUS(bpb, cluster) \
    (false)
#define get_next_cluster \
    get_next_cluster32
#define alloc_cluster \
    alloc_cluster32
#define update_fat_entry \
    update_fat_entry32
#endif /* HAVE_FAT16SUPPORT */

static void update_fsinfo32(struct bpb *bpb);

#ifdef HAVE_FAT16SUPPORT
struct fat_ops
{
    long (*ops_get_next_cluster)(struct bpb *bpb,
                                 long startcluster,
                                 struct iobuf_handle *iobp);
    long (*ops_alloc_cluster)(struct bpb *bpb,
                              long startcluster,
                              struct iobuf_handle *iobp);
    int  (*ops_update_fat_entry)(struct bpb *bpb,
                                 unsigned long entry,
                                 unsigned long val,
                                 struct iobuf_handle *iobp);
};

static long get_next_cluster16(struct bpb *bpb,
                               long startcluster,
                               struct iobuf_handle *iobp);
static long alloc_cluster16(struct bpb *bpb,
                            long startcluster,
                            struct iobuf_handle *iobp);
static int update_fat_entry16(struct bpb *bpb,
                              unsigned long entry,
                              unsigned long val,
                              struct iobuf_handle *iobp);

static const struct fat_ops fat16_ops =
{
    get_next_cluster16,
    alloc_cluster16,
    update_fat_entry16,
};

static long get_next_cluster32(struct bpb *bpb,
                               long startcluster,
                               struct iobuf_handle *iobp);
static long alloc_cluster32(struct bpb *bpb,
                            long startcluster,
                            struct iobuf_handle *iobp);
static int update_fat_entry32(struct bpb *bpb,
                              unsigned long entry,
                              unsigned long val,
                              struct iobuf_handle *iobp);

static const struct fat_ops fat32_ops =
{
    get_next_cluster32,
    alloc_cluster32,
    update_fat_entry32,
};
#endif /* HAVE_FAT16SUPPORT */

/* set error code and jump to routine exit */
#define FAT_ERROR(_rc) \
    ({  __builtin_constant_p(_rc) ?            \
            ({ if (_rc != RC) rc = (_rc); }) : \
            ({ rc = (_rc); });                 \
        goto fat_error; })

static NO_INLINE int fat_error_rc(int rc, int rc2)
{
    /* codec where rc2 % 10 == 0 are a -errno*10 */
    return (rc2 % 10) ? 10 * rc2 + rc : rc2;
}

static NO_INLINE int fat_error_rc_errno(int rc)
{
    /* codes where rc % 10 == 0 are a -errno*10 */
    __internal_errno = rc;
    int c = rc / 10;
    return c*10 == rc ? c : -EIO;
}

#define FAT_ERROR_RC(rc, rc2) \
    ({ int __rc = (rc);       \
       UNLIKELY(__rc < 0) ? fat_error_rc(__rc, (rc2)) : __rc; })

#define FAT_ERROR_RC_ERRNO(rc) \
    ({ int __rc = (rc);        \
       UNLIKELY(__rc < 0) ? fat_error_rc_errno(__rc) : __rc; })


#define FAT_FILESIZE_MAX    UINT32_MAX
#define SECTORNUM_REW       (0ul - 1ul)

enum add_dir_entry_flags
{
    DIRENT_RETURN      = 0x01, /* return the new short entry */
    DIRENT_TEMPL       = 0x0e, /* all TEMPL flags */
    DIRENT_TEMPL_CRT   = 0x02, /* use template crttime */
    DIRENT_TEMPL_WRT   = 0x04, /* use template wrttime */
    DIRENT_TEMPL_ACC   = 0x08, /* use template lstacc time */
    DIRENT_TEMPL_TIMES = 0x0e, /* keep all time fields */
};

struct fatlong_parse_state
{
    int     ord_max;
    int     ord;
    uint8_t chksum;
};

/* Convert OEM name to UTF-8 */
static size_t oem_convert_name(char *d_name, const char *oem_name,
                               size_t len)
{
    /* This MUST be the default codepage thus not something that could be
       loaded on call */
    len = (char *)iso_decode(oem_name, d_name, -1, len) - d_name;
    d_name[len] = '\0';
    return len;
}

static void cache_commit(struct bpb *bpb)
{
    if (!BPB_ISFAT16(bpb))
        update_fsinfo32(bpb);

    iocache_flush_volume(bpb);
}

static FORCE_INLINE void cache_fileop_commit(struct bpb *bpb)
{
#ifdef FS_MIN_WRITECACHING
    /* commit cache after file dirty operations for integrity */
    cache_commit(bpb);
#endif
    (void)bpb;
}

static inline void cache_discard(struct bpb *bpb)
{
    iocache_discard_volume(bpb);
    (void)bpb;
}

/* caches a FAT or data area sector; fetches contents if miss */
static FORCE_INLINE void * cache_sector(struct bpb *bpb,
                                        unsigned long sector,
                                        struct iobuf_handle *iobp,
                                        int priority)
{
    return iobuf_cache(bpb, sector + bpb->startsector, iobp, priority, true);
}

/* returns a raw buffer for a sector; does no fetching */
static FORCE_INLINE void * cache_sector_buffer(struct bpb *bpb,
                                               unsigned long sector,
                                               struct iobuf_handle *iobp,
                                               int priority,
                                               bool fill)
{
    return iobuf_cache(bpb, sector + bpb->startsector, iobp, priority, fill);
}

/* return the previously-accessed sector index for the file offset */
static FORCE_INLINE unsigned long offset_last_secnum(file_size_t offs)
{
    /* overflow proof whereas "(x + y - 1) / y" is not */
    unsigned long secnum = (unsigned long)(offs / SECTOR_SIZE) - 1;

    if (offs % SECTOR_SIZE)
        secnum++;

    return secnum;
}

/* return the last transferred sector number for the stream */
static FORCE_INLINE unsigned long str_last_secnum(const struct filestr *str)
{
    return str->infop->bpb->secperclus*str->clusternum + str->sectornum;
}

static void dosent_set_fstclus(long fstclus, union raw_dirent *ent)
{
    ent->fstclushi = htole16(fstclus >> 16);
    ent->fstcluslo = htole16(fstclus & 0xffff);
}

static uint8_t shortname_checksum(const unsigned char *shortname)
{
    /* calculate shortname checksum */
    uint8_t chksum = 0;

    for (unsigned int i = 0; i < 11; i++)
        chksum = (chksum << 7) + (chksum >> 1) + shortname[i];

    return chksum;
}

static size_t parse_short_direntry(const union raw_dirent *ent,
                                   struct fileentry *entry)
{
    entry->attr         = ent->attr;
    entry->crttimetenth = ent->crttimetenth;
    entry->crttime      = letoh16(ent->crttime);
    entry->crtdate      = letoh16(ent->crtdate);
    entry->lstaccdate   = letoh16(ent->lstaccdate);
    entry->wrttime      = letoh16(ent->wrttime);
    entry->wrtdate      = letoh16(ent->wrtdate);
    entry->filesize     = letoh32(ent->filesize);
    entry->firstcluster = ((uint32_t)letoh16(ent->fstcluslo)      ) |
                          ((uint32_t)letoh16(ent->fstclushi) << 16);
    /* fix the name */
    bool lowercase = ent->ntres & FAT_NTRES_LC_NAME;
    unsigned char c = ent->name[0];

    if (c == 0x05)  /* special kanji char */
        c = 0xe5;

    int j = 0;

    for (int i = 0; c != ' '; c = ent->name[i])
    {
        entry->shortname[j++] = lowercase ? tolower(c) : c;

        if (++i >= 8)
            break;
    }

    if (ent->name[8] != ' ')
    {
        lowercase = ent->ntres & FAT_NTRES_LC_EXT;
        entry->shortname[j++] = '.';

        for (int i = 8; i < 11 && (c = ent->name[i]) != ' '; i++)
            entry->shortname[j++] = lowercase ? tolower(c) : c;
    }

    entry->shortname[j] = 0;
    return j;
}

static void fill_fileentry(const union raw_dirent *ent,
                           const struct fileinfo *info,
                           struct fileentry *entry)
{
    parse_short_direntry(ent, entry);
    entry->direntry   = info->direntry;
    entry->numentries = info->numentries;
}

static unsigned char char2dos(unsigned char c, int *np)
{
    /* FIXME: needs conversion to OEM charset FIRST but there is currently
       no unicode function for that! */

    /* smallest tables with one-step lookup that directly map the lists;
       here we're only concerned with what gets through the longname
       filter (check_longname will have been called earlier so common
       illegal chars are neither in these tables nor checked for) */
    static const unsigned char remove_chars_tbl[3] =
        { 0, '.', ' ' };

    static const unsigned char replace_chars_tbl[11] =
        { ',', 0, 0, '[', ';', ']', '=', 0, 0, 0, '+' };

    if (remove_chars_tbl[c % 3] == c)
    {
        /* Illegal char, remove */
        c = 0;
        *np = 0;
    }
    else if (c >= 0x80 || replace_chars_tbl[c % 11] == c)
    {
        /* Illegal char, replace (note: NTFS behavior for extended chars) */
        c = '_';
        *np = 0;
    }
    else
    {
        c = toupper(c);
    }

    return c;
}

/* convert long name into dos name, possibly recommending randomization */
static void create_dos_name(unsigned char *basisname,
                            const unsigned char *name, int *np)
{
    int i;

    /* FIXME: needs conversion to OEM charset FIRST but there is currently
       no unicode function for that! */

    /* as per FAT spec, set "lossy conversion" flag if any destructive
       alterations to the name occur other than case changes */
    *np = -1;

    /* find extension part */
    unsigned char *ext = strrchr(name, '.');
    if (ext && (ext == name || strchr(ext, ' ')))
        ext = NULL; /* handle .dotnames / extensions cannot have spaces */

    /* name part */
    for (i = 0; *name && (!ext || name < ext) && (i < 8); name++)
    {
        unsigned char c = char2dos(*name, np);
        if (c)
            basisname[i++] = c;
    }

    /* pad both name and extension */
    while (i < 11)
        basisname[i++] = ' ';

    if (basisname[0] == 0xe5) /* special kanji character */
        basisname[0] = 0x05;

    /* extension part */
    if (!ext++)
        return; /* no extension */

    for (i = 8; *ext && i < 11; ext++)
    {
        unsigned char c = char2dos(*ext, np);
        if (c)
            basisname[i++] = c;
    }

    if (*ext)
        *np = 0; /* extension too long */
}

static void randomize_dos_name(unsigned char *dosname,
                               const unsigned char *basisname,
                               int *np)
{
    int n = *np;

    memcpy(dosname, basisname, 11);

    if (n < 0)
    {
        /* first one just copies */
        *np = 0;
        return;
    }

    /* the "~n" string can range from "~1" to "~999999"
       of course a directory can have at most 65536 entries which means
       the numbers will never be required to get that big in order to map
       to a unique name */
    if (++n > 999999)
        n = 1;

    unsigned char numtail[8]; /* holds "~n" */
    unsigned int numtaillen = snprintf(numtail, 8, "~%d", n);

    unsigned int basislen = 0;
    while (basislen < 8 && basisname[basislen] != ' ')
        basislen++;

    memcpy(dosname + MIN(8 - numtaillen, basislen), numtail, numtaillen);

    *np = n;
}

/* check long filename for validity */
static int check_longname(const unsigned char *name)
{
    /* smallest table with one-step lookup that directly maps the list */
    static const unsigned char invalid_chars_tbl[19] =
    {
        0, ':', 0, '<', '*', '>', '?', 0, 0,
        '/', '|', 0, 0, 0x7f, 0, '"', '\\', 0, 0
    };

    if (!name)
        return -1;

    unsigned int c = *name;

    do
    {
        if (c < 0x20 || invalid_chars_tbl[c % 19] == c)
            return -2;
    }
    while ((c = *++name));

    /* check trailing space(s) and periods */
    c = *--name;
    if (c == ' ' || c == '.')
        return -3;

    return 0;
}

/* Get first longname entry name offset */
static inline unsigned int longent_char_first(void)
{
    return 1;
}

/* Get the next longname entry offset or 0 if the end is reached */
static inline unsigned int longent_char_next(unsigned int i)
{
    switch (i += 2)
    {
    case 26: i -= 1; /* return 28 */
    case 11: i += 3; /* return 14 */
    }

    return i < 32 ? i : 0;
}

/* initialize the parse state; call before parsing first long entry */
static void NO_INLINE fatlong_parse_start(struct fatlong_parse_state *lnparse,
                                          struct fileentry *entry)
{
    /* no inline so gcc can't figure out what isn't initialized here;
       ord_max is king as to the validity of all other fields */
    lnparse->ord_max = -1; /* one resync per parse operation */
    entry->direntry   = 0;
    entry->numentries = 0;
}

/* convert the FAT long name entry to a contiguous segment */
static bool fatlong_parse_entry(struct fatlong_parse_state *lnparse,
                                const union raw_dirent *ent,
                                struct fileentry *entry)
{
    int ord = ent->ldir_ord;

    if (ord & FATLONG_ORD_F_LAST)
    {
        /* this entry is the first long entry (first in order but
           containing last part) */
        ord &= ~FATLONG_ORD_F_LAST;

        if (ord == 0 || ord > FATLONG_MAX_ORDER)
        {
            lnparse->ord_max = 0;
            return true;
        }

        lnparse->ord_max = ord;
        lnparse->ord     = ord;
        lnparse->chksum  = ent->ldir_chksum;
    }
    else
    {
        /* valid ordinals yet? */
        if (lnparse->ord_max <= 0)
        {
            if (lnparse->ord_max == 0)
                return true;

            lnparse->ord_max = 0;
            return false; /* try resync */
        }

        /* check ordinal continuity and that the checksum matches the
           one stored in the last entry */
        if (ord == 0 || ord != lnparse->ord - 1 ||
            lnparse->chksum != ent->ldir_chksum)
        {
            lnparse->ord_max = 0;
            return true;
        }
    }

    /* so far so good; save entry information */
    lnparse->ord = ord;

    uint16_t *ucsp = entry->ucssegs[ord - 1];
    unsigned int i = longent_char_first();

    while ((*ucsp++ = BYTES2INT16(ent->data, i)))
    {
        if (!(i = longent_char_next(i)))
            return true;
    }

    /* segment may end early only in last entry */
    if (ord == lnparse->ord_max)
    {
        /* the only valid padding, if any, is 0xffff */
        do
        {
            if (!(i = longent_char_next(i)))
                return true;
        }
        while (BYTES2INT16(ent->data, i) == 0xffff);
    }

    /* long filename is corrupt */
    lnparse->ord_max = 0;
    return true;
}

/* finish parsing of the longname entries and do the conversion to
   UTF-8 if we have all the segments */
static void fatlong_parse_finish(struct fatlong_parse_state *lnparse,
                                 const union raw_dirent *ent,
                                 unsigned int direntry,
                                 struct fileentry *entry)
{
    size_t len = parse_short_direntry(ent, entry);
    unsigned char *name = entry->name;

    entry->direntry = direntry;

    /* ord_max must not have been set to <= 0 because of any earlier problems
       and the ordinal immediately before the shortname entry must be 1 */
    if (lnparse->ord_max <= 0 || lnparse->ord != 1)
        goto is_dos;

     /* check the longname checksums against the shortname checksum */
    if (lnparse->chksum != shortname_checksum(ent->name))
        goto is_dos;

    /* longname is good so far so convert all the segments to UTF-8 */
    unsigned char *p = name;

    /* ensure the last segment is NULL-terminated if it is filled */
    entry->ucssegs[lnparse->ord_max][0] = 0x0000;

    for (uint16_t *ucsp = entry->ucssegs[0], ucc = *ucsp;
         ucc; ucc = *++ucsp)
    {
        /* end should be hit before ever seeing padding */
        if (ucc == 0xffff)
            goto is_dos;

        if ((p = utf8encode(ucc, p)) - name > FAT_UTF8_NAME_MAX)
            goto is_dos;
    }

    /* longname ok */
    *p = '\0';
    entry->namelen = p - name;
    entry->numentries = lnparse->ord_max + 1;
    DEBUGF("LN:'%s'\n", entry->name);
    return;
is_dos:
    /* the long entries failed to pass all checks or there is
    just a short entry. */
    entry->namelen = oem_convert_name(name, entry->shortname, len);
    entry->numentries = 1;
    DEBUGF("SN-DOS:'%s'\n", entry->name);
}

static unsigned long cluster2sec(struct bpb *bpb,
                                 long cluster)
{
    if (!IS_FAT16_ROOTCLUS(bpb, cluster) && (cluster -= 2) >= bpb->dataclusters)
    {
        DEBUGF("%s:Bad cluster number:%ld\n", __func__, cluster + 2);
        return 0;
    }

    return cluster*bpb->secperclus + bpb->firstdatasector;
}

static int fat_mirror_wb_cb(struct bpb *bpb,
                            unsigned long sector,
                            int count,
                            const void *buf)
{
    for (unsigned int i = bpb->numfats; i; i--)
    {
        int rc = iocache_writeback(bpb, sector, count, buf);
        if (rc < 0)
            return rc;

        sector += bpb->fatsz;
    }

    return 0;
}

static void fat_recalc_free(struct bpb *bpb)
{
    struct iobuf_handle iob;
    iobuf_init(&iob);

    long free = 0;

    const bool isfat16 = BPB_ISFAT16(bpb);
    unsigned long sector = bpb->fatrgnstart;
    void *sec = NULL;

    __fsinfo_lock(bpb);

    for (long c = 2; c < bpb->dataclusters + 2; c++)
    {
        unsigned long offset = isfat16 ?
                               ((unsigned long)c % CLUSTERS_PER_FAT16_SECTOR) :
                               ((unsigned long)c % CLUSTERS_PER_FAT32_SECTOR);
        if (!offset && sec)
        {
            iobuf_release(&iob);
            sec = NULL;
        }

        __fsinfo_unlock(bpb);

        if (!sec)
            sec = cache_sector(bpb, sector++, &iob, SECPRIO_DATA);

        __fsinfo_lock(bpb);

        if (!sec)
        {
            free = -1;
            break;
        }

        unsigned long val_le = isfat16 ? ((uint16_t *)sec)[offset] :
                                         ((uint32_t *)sec)[offset];

        if (!(val_le & htole32(0x0fffffff)))
        {
            bpb->fsinfo.dirty = true;

            if (bpb->fsinfo.nextfree == -1)
                bpb->fsinfo.nextfree = c;

            free++;
        }
    }

    if (free != -1)
        bpb->fsinfo.freecount = free;

    if (sec)
        iobuf_release(&iob);

    __fsinfo_unlock(bpb);

    if (!isfat16)
        update_fsinfo32(bpb);
}

#ifdef HAVE_FAT16SUPPORT
static long get_next_cluster16(struct bpb *bpb,
                               long startcluster,
                               struct iobuf_handle *iobp)
{
    if (IS_FAT16_ROOTCLUS(bpb, startcluster))
        return startcluster + 1; /* FAT16 root dir; dont use the FAT */

    unsigned long entry = startcluster;
    unsigned long sector = entry / CLUSTERS_PER_FAT16_SECTOR;
    unsigned long offset = entry % CLUSTERS_PER_FAT16_SECTOR;

    uint16_t *sec = cache_sector(bpb, sector + bpb->fatrgnstart, iobp,
                                 SECPRIO_FAT);
    if (!sec)
    {
        DEBUGF("%s: Could not cache sector %lu\n", __func__, sector);
        return -1;
    }

    long next = letoh16(sec[offset]);
    iobuf_release(iobp);

    /* is this last cluster in chain? */
    if (next >= FAT16_EOF_MARK)
        next = 0;

    return next;
}

static long alloc_cluster16(struct bpb *bpb,
                            long startcluster,
                            struct iobuf_handle *iobp)
{
    long cstart = startcluster;
    long climit = bpb->dataclusters + 2;

    if (cstart < 2 || cstart >= climit)
        cstart = 2;

    unsigned long sector = cstart / CLUSTERS_PER_FAT16_SECTOR
                                + bpb->fatrgnstart;
    uint16_t *sec = NULL;

    while (1)
    {
        for (long c = cstart; c < climit; c++)
        {
            unsigned long offset = (unsigned long)c % CLUSTERS_PER_FAT16_SECTOR;
            if (!offset && sec)
            {
                iobuf_release(iobp);
                sec = NULL;
            }

            if (!sec)
            {
                sec = cache_sector(bpb, sector++, iobp, SECPRIO_FAT);
                if (!sec)
                    return -1;
            }

            if (sec[offset] != 0x0000)
                continue;

            unsigned long newval_le = htole16(FAT16_EOF_MARK);
            if (__fat_cas16(&sec[offset], 0x0000, newval_le) != 0x0000)
                continue;

            iobuf_release_dirty_cb(iobp);
            fsinfo_freecount_dec(bpb, c);

            DEBUGF("%s:st=%ld:c=%ld\n", __func__, startcluster, c);
            return c;
        }

        if (cstart == 2)
            break;

        climit = cstart;
        cstart = 2;
        sector = bpb->fatrgnstart;
    }

    if (sec)
        iobuf_release(iobp);

    DEBUGF("%s:st=%ld:c=0 (DISK_FULL)\n", __func__, startcluster);
    return 0; /* 0 is an illegal cluster number */
}

static int update_fat_entry16(struct bpb *bpb,
                              unsigned long entry,
                              unsigned long val,
                              struct iobuf_handle *iobp)
{
    unsigned long sector = entry / CLUSTERS_PER_FAT16_SECTOR;
    unsigned long offset = entry % CLUSTERS_PER_FAT16_SECTOR;

    val &= 0xFFFF;

    DEBUGF("%s:entry=%lu:val=%04lX\n", __func__, entry, val);

    if (entry == val)
        panicf("FAT16 loop:%lu", entry);

    if (entry < 2)
        panicf("FAT16 reserved:%lu", entry);

    uint16_t *sec = cache_sector(bpb, sector + bpb->fatrgnstart, iobp,
                                 SECPRIO_FAT);
    if (!sec)
    {
        DEBUGF("Could not cache sector %lu\n", sector);
        return -1;
    }

    unsigned long curval_le = sec[offset];
    if (!curval_le)
    {
        /* only alloc_cluster16 may write an unclaimed entry */
        DEBUGF("%s:allocations forbidden here\n", __func__);
        panicf("ufe16:alloc!");
        return -2;
    }

    unsigned long newval_le = htole16(val);
    sec[offset] = newval_le;
    iobuf_release_dirty_cb(iobp);

    if (!val)
        fsinfo_freecount_inc(bpb);

    DEBUGF("%ld free clusters\n", bpb->fsinfo.freecount);
    return 0;
}
#endif /* HAVE_FAT16SUPPORT */

static void update_fsinfo32(struct bpb *bpb)
{
    __fsinfo_lock(bpb);

    if (bpb->fsinfo.dirty)
    {
        __fsinfo_unlock(bpb);

        struct iobuf_handle iob;
        iobuf_init(&iob);
        unsigned char *sec = cache_sector(bpb, bpb->fsinfosec, &iob,
                                          SECPRIO_DATA);
        if (!sec)
            return;

        __fsinfo_lock(bpb);
        if (bpb->fsinfo.dirty)
        {
            INT322BYTES(sec, FSINFO_FREECOUNT, bpb->fsinfo.freecount);
            INT322BYTES(sec, FSINFO_NEXTFREE, bpb->fsinfo.nextfree);
            bpb->fsinfo.dirty = false;
            iobuf_release_dirty(&iob);
        }
        else
        {
            iobuf_release(&iob);
        }
    }

    __fsinfo_unlock(bpb);
}

static long get_next_cluster32(struct bpb *bpb,
                               long startcluster,
                               struct iobuf_handle *iobp)
{
    unsigned long entry = startcluster;
    unsigned long sector = entry / CLUSTERS_PER_FAT32_SECTOR;
    unsigned long offset = entry % CLUSTERS_PER_FAT32_SECTOR;

    uint32_t *sec = cache_sector(bpb, sector + bpb->fatrgnstart, iobp,
                                 SECPRIO_FAT);
    if (!sec)
    {
        DEBUGF("%s: Could not cache sector %lu\n", __func__, sector);
        return -1;
    }

    long next = letoh32(sec[offset]) & 0x0fffffff;
    iobuf_release(iobp);

    /* is this last cluster in chain? */
    if (next >= FAT32_EOF_MARK)
        next = 0;

    return next;
}

static long alloc_cluster32(struct bpb *bpb,
                            long startcluster,
                            struct iobuf_handle *iobp)
{
    long cstart = startcluster;
    long climit = bpb->dataclusters + 2;

    if (cstart < 2 || cstart >= climit)
        cstart = 2;

    unsigned long sector = cstart / CLUSTERS_PER_FAT32_SECTOR
                                + bpb->fatrgnstart;
    uint32_t *sec = NULL;

    while (1)
    {
        for (long c = cstart; c < climit; c++)
        {
            unsigned long offset = (unsigned long)c % CLUSTERS_PER_FAT32_SECTOR; 
            if (!offset && sec)
            {
                iobuf_release(iobp);
                sec = NULL;
            }

            if (!sec)
            {
                sec = cache_sector(bpb, sector++, iobp, SECPRIO_FAT);
                if (!sec)
                    return -1;
            }

            unsigned long curval_le = sec[offset];
            if (sec[offset] & htole32(0x0fffffff))
                continue;

            curval_le &= htole32(0xf0000000);

            unsigned long newval_le = htole16(FAT32_EOF_MARK) | curval_le;
            if (__fat_cas32(&sec[offset], curval_le, newval_le) != curval_le)
                continue;

            iobuf_release_dirty_cb(iobp);
            fsinfo_freecount_dec(bpb, c);

            DEBUGF("%s:st=%ld:c=%ld\n", __func__, startcluster, c);
            return c;
        }

        if (cstart == 2)
            break;

        climit = cstart;
        cstart = 2;
        sector = bpb->fatrgnstart;
    }

    if (sec)
        iobuf_release(iobp);

    DEBUGF("%s:st=%ld:c=0 (DISK FULL)\n", __func__, startcluster);
    return 0; /* 0 is an illegal cluster number */
}

static int update_fat_entry32(struct bpb *bpb,
                              unsigned long entry,
                              unsigned long val,
                              struct iobuf_handle *iobp)
{
    unsigned long sector = entry / CLUSTERS_PER_FAT32_SECTOR;
    unsigned long offset = entry % CLUSTERS_PER_FAT32_SECTOR;

    DEBUGF("%s:entry=%lu:val=%08lX\n", __func__, entry, val);

    if (entry == val)
        panicf("FAT32 loop:%lu", entry);

    if (entry < 2)
        panicf("FAT32 reserved:%lu", entry);

    uint32_t *sec = cache_sector(bpb, sector + bpb->fatrgnstart, iobp,
                                 SECPRIO_FAT);
    if (!sec)
    {
        DEBUGF("Could not cache sector %lu\n", sector);
        return -1;
    }

    unsigned long curval_le = sec[offset];
    if (!(curval_le & htole32(0x0fffffff)))
    {
        /* only alloc_cluster32 may write an unclaimed entry */
        DEBUGF("%s:allocations forbidden here\n", __func__);
        panicf("ufe32:alloc!");
        return -2;
    }

    /* don't change top 4 bits */
    unsigned long newval_le = htole32(val & 0x0fffffff) |
                              (curval_le & htole32(0xf0000000));
    sec[offset] = newval_le;
    iobuf_release_dirty_cb(iobp);

    if (!val)
        fsinfo_freecount_inc(bpb);

    DEBUGF("%ld free clusters\n", bpb->fsinfo.freecount);
    return 0;
}

static int bpb_is_sane(const struct fatbpb *fatbpb)
{

    if (fatbpb->bpb_lastword != 0xaa55)
    {
        DEBUGF("%s:BPB_LAST_WORD != 0xaa55 "
               "(0x%04X)\n", __func__,
               (unsigned)fatbpb->bpb_lastword);
        return -1;
    }

    switch (fatbpb->bpb_bytspersec)
    {
    case 512: case 1024: case 2048: case 4096:
        /* these are the only valid values */
        break;
    default:
        DEBUGF("%s: Error: sector size is not sane "
               "(%u)\n", __func__,
               (unsigned)fatbpb->bpb_bytspersec);
        return -2;
    }

    unsigned long clussz = (unsigned long)fatbpb->bpb_secperclus
                                * fatbpb->bpb_bytspersec;
    if (clussz > 128*1024ul)
    {
        DEBUGF("%s: Error: cluster size is larger than 128K "
               "(%lu * %lu = %lu)\n", __func__,
               (unsigned long)fatbpb->bpb_bytspersec,
               (unsigned long)fatbpb->bpb_secperclus,
               clussz);
        return -3;
    }

    if (fatbpb->bpb_numfats != 2)
    {
        DEBUGF("%s: Warning: NumFATS is not 2 "
               "(%u)\n", __func__,
               (unsigned)fatbpb->bpb_numfats);
    }

    if (!fatbpb->bpb_isfat16 && (fatbpb->bpb_extflags & 0x0080) &&
        (fatbpb->bpb_extflags & 0x000f) >= fatbpb->bpb_numfats)
    {
        DEBUGF("%s: Active FAT is past all FATs! "
               "(%u of %u)\n", __func__,
               (unsigned)(fatbpb->bpb_extflags & 0x000f),
               (unsigned)fatbpb->bpb_numfats);
        return -4;
    }

    if (fatbpb->bpb_media != 0xf0 && fatbpb->bpb_media < 0xf8)
    {
        DEBUGF("%s:Warning: Non-standard media type "
               "(0x%02X)\n", __func__,
               (unsigned)fatbpb->bpb_media);
    }

    return 0;
}

static int fat_mount(struct bpb *bpb)
{
    /* safe to grab buffer: bpb is irrelevant and no sector will be cached
       for this volume since it isn't mounted */
    unsigned char *buf = iocache_get_buffer();
    if (!buf)
    {
        DEBUGF("%s: Failed to get buffer\n", __func__);
        return -1;
    }

    int rc, rc2 = 0;

    /* Read the sector */
    rc2 = storage_read_sectors(IF_MD(bpb->drive,) bpb->startsector, 1, buf);
    if(rc2 < 0)
    {
        DEBUGF("%s: Couldn't read BPB:rc=%d\n", __func__, rc2);
        FAT_ERROR(-2);
    }

    struct fatbpb fatbpb;

    fatbpb.bpb_bytspersec = BYTES2INT16(buf, BPB_BYTSPERSEC);
    unsigned long secmult = fatbpb.bpb_bytspersec / SECTOR_SIZE;

    fatbpb.bpb_secperclus = secmult * buf[BPB_SECPERCLUS];
    fatbpb.bpb_rsvdseccnt = secmult * BYTES2INT16(buf, BPB_RSVDSECCNT);
    fatbpb.bpb_numfats    = buf[BPB_NUMFATS];
    fatbpb.bpb_media      = buf[BPB_MEDIA];
    fatbpb.bpb_fatsz16    = secmult * BYTES2INT16(buf, BPB_FATSZ16);
    fatbpb.bpb_fatsz32    = secmult * BYTES2INT32(buf, BPB_FATSZ32);
    fatbpb.bpb_extflags   = BYTES2INT16(buf, BPB_EXTFLAGS);
    fatbpb.bpb_totsec16   = secmult * BYTES2INT16(buf, BPB_TOTSEC16);
    fatbpb.bpb_totsec32   = secmult * BYTES2INT32(buf, BPB_TOTSEC32);
    fatbpb.bpb_lastword   = BYTES2INT16(buf, BPB_LAST_WORD);
    fatbpb.bpb_isfat16    = false;

    /* These are used in calculations below */
    if (!fatbpb.bpb_bytspersec)
    {
        DEBUGF("%s:bpb_bytespersec is 0\n", __func__);
        FAT_ERROR(-3);
    }

    if (!fatbpb.bpb_secperclus)
    {
        DEBUGF("%s:bpb_secperclus is 0\n", __func__);
        FAT_ERROR(-4);
    }

    /* calculate a few commonly used values */
    bpb->secperclus  = fatbpb.bpb_secperclus;
    bpb->clussize    = bpb->secperclus * SECTOR_SIZE;
    bpb->fatsz       = fatbpb.bpb_fatsz16 ?: fatbpb.bpb_fatsz32;
    bpb->numfats     = fatbpb.bpb_numfats;
    bpb->fatrgnstart = fatbpb.bpb_rsvdseccnt;
    bpb->totsec      = fatbpb.bpb_totsec16 ?: fatbpb.bpb_totsec32;

    unsigned long rootdirsectors = 0;
#ifdef HAVE_FAT16SUPPORT
    unsigned long rootentcnt = BYTES2INT16(buf, BPB_ROOTENTCNT);
    rootdirsectors = secmult * ((rootentcnt * DIR_ENTRY_SIZE
                        + fatbpb.bpb_bytspersec - 1) / fatbpb.bpb_bytspersec);
#endif /* HAVE_FAT16SUPPORT */

    bpb->firstdatasector = fatbpb.bpb_rsvdseccnt + bpb->numfats * bpb->fatsz
                                + rootdirsectors;
    bpb->dataclusters = (bpb->totsec - bpb->firstdatasector) / bpb->secperclus;

    /* Determine FAT type */
    if (bpb->dataclusters < 65525)
    {
#ifdef HAVE_FAT16SUPPORT
        if (bpb->dataclusters < 4085)
        {
            /* FAT12 */
            DEBUGF("This is FAT12. Go away!\n");
            FAT_ERROR(-5);
        }

        DEBUGF("Type is FAT16\n");
        fatbpb.bpb_isfat16 = true;
#else /* !HAVE_FAT16SUPPORT */
        DEBUGF("This is not FAT32. Go away!\n");
        FAT_ERROR(-5);
#endif /* HAVE_FAT16SUPPORT */
    }

    if (fatbpb.bpb_isfat16)
    {
        /* FAT16 specific part of BPB */
        /* rounded up, to full clusters */
        long dirclusters =
            (rootdirsectors + bpb->secperclus - 1) / bpb->secperclus;
        /* I assign negative pseudo cluster numbers for the root directory,
           their range is counted upward until -1. */
        bpb->rootclus = 0 - dirclusters; /* backwards, before the data */
        bpb->rootdirsectornum = dirclusters * bpb->secperclus - rootdirsectors;
        bpb->firstallowed = fatbpb.bpb_rsvdseccnt
            + (unsigned long)bpb->numfats * fatbpb.bpb_fatsz16;
        SET_FAT_OPS(bpb, &fat16_ops);
    }
    else
    {
        /* FAT32 specific part of BPB */
        bpb->rootclus = BYTES2INT32(buf, BPB_ROOTCLUS);
        bpb->fsinfosec = secmult * BYTES2INT16(buf, BPB_FSINFO);
        bpb->firstallowed = bpb->firstdatasector;
        SET_FAT_OPS(bpb, &fat32_ops);
    }

    rc2 = bpb_is_sane(&fatbpb);
    if (rc2 < 0)
    {
        DEBUGF("%s: BPB is insane!\n", __func__);
        FAT_ERROR(-6);
    }

    if (fatbpb.bpb_isfat16)
    {
        /* Force recalc later */
        bpb->fsinfo.freecount = -1;
        bpb->fsinfo.nextfree  = -1;
    }
    else
    {
        if (fatbpb.bpb_extflags & 0x0080)
        {
            /* FAT mirroring inactive - read and write only the active one */
            bpb->numfats = 1;
            bpb->fatrgnstart += bpb->fatsz*(fatbpb.bpb_extflags & 0x000f);
        }

        /* Read the fsinfo sector */
        rc2 = storage_read_sectors(IF_MD(bpb->drive,)
                bpb->startsector + bpb->fsinfosec, 1, buf);
        if (rc2 < 0)
        {
            DEBUGF("%s:Couldn't read FSInfo:rc=%d\n", __func__, rc2);
            FAT_ERROR(-7);
        }

        bpb->fsinfo.freecount = BYTES2INT32(buf, FSINFO_FREECOUNT);
        bpb->fsinfo.nextfree  = BYTES2INT32(buf, FSINFO_NEXTFREE);
    }

    rc = 0;
fat_error:
    iocache_release_buffer(buf);
    return FAT_ERROR_RC(rc, rc2);
}

static long next_write_cluster(struct bpb *bpb,
                               long oldcluster,
                               struct iobuf_handle *iobp)
{
    DEBUGF("%s:oldclus=%ld\n", __func__, oldcluster);

    long cluster = 0;

    /* cluster already allocated? */
    if (oldcluster)
        cluster = get_next_cluster(bpb, oldcluster, iobp);

    if (!cluster)
    {
        /* passed end of existing entries and now need to append */
        if (IS_FAT16_ROOTCLUS(bpb, oldcluster))
            return -1; /* impossible to append something to the root */

        unsigned long findstart = (oldcluster ?: bpb->fsinfo.nextfree) + 1;

        /* atomically extend the cluster chain by writing the new EOF mark
           then updating the previous one to point to it */
        cluster = alloc_cluster(bpb, findstart, iobp);
        if (cluster < 0)
        {
            DEBUGF("Error allocating new cluster!\n");
            return -2;
        }
        else if (cluster && oldcluster)
        {
            if (update_fat_entry(bpb, oldcluster, cluster, iobp))
                return -3; /* something's wrong; abandon it; do fsck */
        }
    }

    return cluster;
}

static void fat_rewind(struct filestr *str)
{
    /* rewind the FAT stream's position */
    str->lastcluster = str->infop ? str->infop->firstcluster : 0;
    str->lastsector  = 0;
    str->clusternum  = 0;
    str->sectornum   = SECTORNUM_REW;
    str->eof         = false;
}

static void fat_filestr_init(struct fileinfo *info,
                             struct filestr *str)
{
    filestr_hdr_init(info, str);
    iobuf_init(&str->fatiob);
    fat_rewind(str);
}

static void fat_fileinfo_init(struct bpb *bpb,
                              long startcluster,
                              struct fileinfo *info,
                              struct filestr *str)
{
    fileinfo_hdr_init(bpb, 0, info);
    info->firstcluster = startcluster;
    info->dircluster   = 0;
    info->direntry     = 0;
    info->numentries   = 0;

    if (str)
        fat_filestr_init(info, str);
}

static int fat_seek(struct filestr *str,
                    unsigned long seeksector)
{
    DEBUGF("%s:str=%p:sksec=%lu\n", __func__, str, seeksector);

    struct fileinfo *info = str->infop;
    struct bpb *bpb = info->bpb;

    int rc = 0;
    long          cluster    = info->firstcluster;
    unsigned long sector     = 0;
    long          clusternum = 0;
    unsigned long sectornum  = SECTORNUM_REW;

    if (IS_FAT16_ROOTCLUS(bpb, cluster))
        seeksector += bpb->rootdirsectornum;

    if (seeksector)
    {
        if (cluster == 0)
        {
            DEBUGF("Seeking beyond the end of empty file! "
                   "(sec=%lu:clus=%lu)\n", seeksector,
                   seeksector / bpb->secperclus);
            FAT_ERROR(-1); /* stay at start, set eof */
        }

        /* we need to find the sector BEFORE the requested, since
           the file struct stores the last accessed sector */
        seeksector--;
        clusternum = seeksector / bpb->secperclus;
        sectornum = seeksector % bpb->secperclus;

        long numclusters = clusternum;

        if (str->clusternum && clusternum >= str->clusternum)
        {
            /* seek forward from current position */
            cluster = str->lastcluster;
            numclusters -= str->clusternum;
        }

        for (long i = 0; i < numclusters; i++)
        {
            long last = cluster;
            long next = get_next_cluster(info->bpb, last, &str->fatiob);
            if (next > 0)
            {
                cluster = next;
                continue;
            }

            if (next < 0)
                FAT_ERROR(next * 10 - 2);

            DEBUGF("Seeking beyond the end of the file! "
                   "(sec=%lu:clus=%ld)\n", seeksector, i);
            /* fix-up the stream to point at the final sector */
            cluster = last;
            clusternum -= numclusters - i;
            sectornum = bpb->secperclus - 1;
            rc = -1; /* DO update position to last good and set eof */
            break;
        }

        sector = cluster2sec(bpb, cluster) + sectornum;
    }

    DEBUGF("%s:fstclus=%ld:clus=%ld:sec=%lu:secnum=%lu\n", __func__,
           info->firstcluster, cluster, sector, sectornum);

    str->lastcluster = cluster;
    str->lastsector  = sector;
    str->clusternum  = clusternum;
    str->sectornum   = sectornum;
fat_error:
    str->eof         = rc == -1;
    return rc;
}

static int fat_seek_offset(struct filestr *str,
                           file_size_t offset)
{
    return fat_seek(str, offset_last_secnum(offset) + 1);
}

/* caches a sequential sector number for a stream; fetches contents if miss */
static void * cache_sector_num(struct filestr *str,
                               unsigned long secnum,
                               int priority)
{
    str->eof = false;

    if (str_last_secnum(str) != secnum)
    {
        int rc = fat_seek(str, secnum + 1);
        if (rc < 0)
            return NULL;
    }

    return cache_sector(str->infop->bpb, str->lastsector, &str->iob,
                        priority);
}

/* helper for fat_readwrite */
static long transfer(struct bpb *bpb,
                     unsigned long start,
                     long count,
                     char *buf,
                     bool write)
{
    DEBUGF("%s:st=%lu:cnt=%ld:wr=%d\n", __func__,
           start + bpb->startsector, count, (int)write);

    if (write)
    {
        if (start < bpb->firstallowed)
            panicf("Write %lu before data\n", bpb->firstallowed - start);

        if (start + count > bpb->totsec)
            panicf("Write %lu after data\n", start + count - bpb->totsec);
    }

    int rc = iocache_readwrite(bpb, start + bpb->startsector, count, buf, write);
    if (rc < 0)
    {
        DEBUGF("Couldn't %s sector %lu (err %d)\n",
               write ? "write":"read", start, rc);
        rc = rc * 10 - 1;
    }

    return rc;
}

static int fat_readwrite(struct filestr *str,
                         unsigned long *countp,
                         void *buf,
                         bool write)
{
    unsigned long sectorcount = *countp;
    bool eof = str->eof;

    if ((eof && !write) || !sectorcount)
        return 0;

    unsigned long transferred = 0;
    int rc, rc2 = 0;

    struct fileinfo *info = str->infop;
    struct bpb *bpb = info->bpb;

    long          cluster    = str->lastcluster;
    unsigned long sector     = str->lastsector;
    long          clusternum = str->clusternum;
    unsigned long sectornum  = str->sectornum;

    DEBUGF("%s:str=%p:cnt=%lu:buf=%p:wr=%d\n", __func__,
           str, sectorcount, buf, (int)write);
    DEBUGF("%s:sec=%lu secnum=%lu:eof=%d\n", __func__,
           sector, sectornum, (int)eof);

    eof = false;

    if (!sector)
    {
        /* look up first sector of file */
        long newcluster = info->firstcluster;

        if (write && !newcluster)
        {
            /* file is empty; try to allocate its first cluster */
            rc2 = next_write_cluster(bpb, 0, &str->fatiob);
            if (rc2 < 0)
                FAT_ERROR(-1);

            newcluster = rc2;
            info->firstcluster = newcluster;
        }

        if (newcluster)
        {
            cluster = newcluster;
            if ((sector = cluster2sec(bpb, cluster)))
            {
                sector--;

                if (IS_FAT16_ROOTCLUS(bpb, info->firstcluster))
                {
                    sector += bpb->rootdirsectornum;
                    sectornum = bpb->rootdirsectornum;
                }
            }
        }
    }

    if (!sector)
    {
        sectorcount = 0;
        eof = true;
    }

    unsigned long count = 0;
    unsigned long last = sector;

    while (transferred + count < sectorcount)
    {
        if (++sectornum >= bpb->secperclus)
        {
            /* out of sectors in this cluster; get the next cluster */
            rc2 = write ? next_write_cluster(bpb, cluster, &str->fatiob) :
                          get_next_cluster(bpb, cluster, &str->fatiob);
            if (rc2 <= 0)
            {
                sectornum--; /* remain in previous position */

                if (rc2 < 0)
                    FAT_ERROR(-2);

                eof = true;
                break;
            }

            cluster = rc2;
            sector = cluster2sec(bpb, cluster) - 1;
            clusternum++;
            sectornum = 0;

            /* jumped clusters right at start? */
            if (!count)
                last = sector;
        }

        /* find sequential sectors and transfer them all at once */
        if (sector != last || count >= FAT_MAX_TRANSFER_SIZE)
        {
            /* not sequential/over limit */
            if (buf)
            {
                rc2 = transfer(bpb, last - count + 1, count, buf, write);
                if (rc2 < 0)
                    FAT_ERROR(-3);
            }

            transferred += count;
            buf += count * SECTOR_SIZE;
            count = 0;
        }

        count++;
        last = ++sector;
    }

    if (count)
    {
        /* transfer any remainder */
        if (buf)
        {
            rc2 = transfer(bpb, last - count + 1, count, buf, write);
            if (rc2 < 0)
                FAT_ERROR(-4);
        }

        transferred += count;
    }

    if (eof && write)
    {
        rc2 = -ENOSPC*10;
        FAT_ERROR(-5);
    }

    rc = 0;
fat_error:
    DEBUGF("%s:transferred=%lu\n", __func__, transferred);
    *countp = transferred;
    str->lastcluster = cluster;
    str->lastsector  = sector;
    str->clusternum  = clusternum;
    str->sectornum   = sectornum;
    str->eof         = eof;

    return FAT_ERROR_RC(rc, rc2);
}

static int free_cluster_chain(struct bpb *bpb,
                              long start,
                              struct iobuf_handle *iobp)
{
    for (long last = start, next; last; last = next, start = 0)
    {
        next = get_next_cluster(bpb, last, iobp);
        int rc = update_fat_entry(bpb, last, 0, iobp);
        if (rc < 0 || next < 0)
            return start ? -1 : 0;
    }

    return 1;
}

/* shorten the file to *newsize bytes */
static int fat_shorten_file(struct filestr *str,
                            file_size_t oldsize,
                            file_size_t *newsize)
{
    /* str must have been seeked to *newsize first */
    DEBUGF("%s:lstclus=%lu\n", __func__, str->lastcluster);

    if (*newsize > oldsize)
        DEBUGF("  %lu>%lu\n", *newsize, oldsize);

    int rc, rc2 = 0;

    struct fileinfo *info = str->infop;
    struct bpb *bpb = info->bpb;
    long last = str->lastcluster;
    long next = 0;

    /* truncate trailing clusters after the current position */
    if (last)
    {
        next = get_next_cluster(bpb, last, &str->fatiob);
        rc2 = update_fat_entry(bpb, last, FAT_EOF_MARK, &str->fatiob);
        if (rc2 < 0)
        {
            DEBUGF("Failed truncating file\n");
            *newsize = oldsize;
            FAT_ERROR(-1);
        }

        /* if we get here, the file is shortened even if the subsequent
           operation fails */
    }

    rc2 = free_cluster_chain(bpb, next, &str->fatiob);
    if (rc2 < 0)
        DEBUGF("Failed freeing remaining cluster chain\n");

    rc = 0;
fat_error:
    return FAT_ERROR_RC(rc, rc2);
}

/* extend the file by *nbyte bytes from the current position */
static int fat_extend_file(struct filestr *str,
                           file_size_t oldsize,
                           file_size_t *newsize)
{
    /* str must have been seeked to oldsize first */
    DEBUGF("%s:lstclus=%lu\n", __func__, str->lastcluster);

    if (*newsize <= oldsize)
        DEBUGF("  %lu<%lu\n", *newsize, oldsize);

    int rc, rc2 = 0;

    file_size_t rem = *newsize - oldsize;

    /* lead bytes never add new clusters */
    unsigned long secoffs = oldsize % SECTOR_SIZE;
    if (secoffs)
    {
        file_size_t amt = MIN(rem, SECTOR_SIZE - secoffs);
        void *buf = cache_sector(str->infop->bpb, str->lastsector,
                                 &str->iob, SECPRIO_DATA);
        if (!buf)
            FAT_ERROR(-1);

        memset(buf + secoffs, 0, amt);
        iobuf_release_dirty(&str->iob);
        rem -= amt;
    }

    /* do any trailing bytes as a whole sector which will allocate any needed
       additional cluster for them */
    unsigned long count = (rem + SECTOR_SIZE - 1) / SECTOR_SIZE;
    if (count)
    {
        void *buf = iocache_get_buffer();
        if (!buf)
            FAT_ERROR(-2);

        memset(buf, 0, SECTOR_SIZE);

        do
        {
            rc2 = fat_readwrite(str, &(unsigned long){ 1 }, buf, true);
            if (rc2 < 0)
                break;

            rem -= MIN(rem, SECTOR_SIZE);
        }
        while (--count);

        iocache_release_buffer(buf);

        if (rc2 < 0)
            FAT_ERROR(-3);
    }

    rc = 0;
fat_error:
    /* return what we managed to do, even if failing */
    if (rem)
        *newsize -= rem;

    return FAT_ERROR_RC(rc, rc2);
}

static union raw_dirent * cache_direntry(struct filestr *str,
                                         unsigned int direntry)
{
    if (direntry >= MAX_DIRENTRIES)
    {
        DEBUGF("%s:de=%u:Dir is too large\n", __func__, direntry);
        str->eof = true;
        return NULL;
    }

    unsigned long secnum = direntry / DIR_ENTRIES_PER_SECTOR;
    union raw_dirent *ent = cache_sector_num(str, secnum, SECPRIO_DIRENT);

    if (ent)
        ent += direntry % DIR_ENTRIES_PER_SECTOR;

    return ent;
}

#if CONFIG_RTC
static void fat_time(uint16_t *date,
                     uint16_t *time,
                     int16_t *tenth)
{
    struct tm *tm = get_time();

    if (date)
    {
        *date = ((tm->tm_year - 80) << 9) |
                ((tm->tm_mon + 1) << 5) |
                tm->tm_mday;
    }

    if (time)
    {
        *time = (tm->tm_hour << 11) |
                (tm->tm_min << 5) |
                (tm->tm_sec >> 1);
    }

    if (tenth)
        *tenth = (tm->tm_sec & 1) * 100;
}

#else /* !CONFIG_RTC */

static void fat_time(uint16_t *date,
                     uint16_t *time,
                     int16_t *tenth)
{
    /* non-RTC version returns an increment from the supplied time, or a
     * fixed standard time/date if no time given as input */

    /* Macros to convert a 2-digit string to a decimal constant.
       (YEAR), MONTH and DAY are set by the date command, which outputs
       DAY as 00..31 and MONTH as 01..12. The leading zero would lead to
       misinterpretation as an octal constant. */
    #define S100(x) 1 ## x
    #define C2DIG2DEC(x) (S100(x)-100)
    /* The actual build date, as FAT date constant */
    #define BUILD_DATE_FAT (((YEAR - 1980) << 9) \
                            | (C2DIG2DEC(MONTH) << 5) \
                            | C2DIG2DEC(DAY))

    bool date_forced = false;
    bool next_day = false;
    unsigned time2 = 0; /* double time, for CRTTIME with 1s precision */

    if (date && *date < BUILD_DATE_FAT)
    {
        *date = BUILD_DATE_FAT;
        date_forced = true;
    }

    if (time)
    {
        time2 = *time << 1;
        if (time2 == 0 || date_forced)
        {
            time2 = (11 < 6) | 11; /* set to 00:11:11 */
        }
        else
        {
            unsigned mins  = (time2 >> 6) & 0x3f;
            unsigned hours = (time2 >> 12) & 0x1f;

            mins = 11 * ((mins / 11) + 1); /* advance to next multiple of 11 */
            if (mins > 59)
            {
                mins = 11; /* 00 would be a bad marker */
                if (++hours > 23)
                {
                    hours = 0;
                    next_day = true;
                }
            }
            time2 = (hours << 12) | (mins << 6) | mins; /* secs = mins */
        }
        *time = time2 >> 1;
    }

    if (tenth)
        *tenth = (time2 & 1) * 100;

    if (date && next_day)
    {
        static const unsigned char daysinmonth[] =
            { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        unsigned day   = *date & 0x1f;
        unsigned month = (*date >> 5) & 0x0f;
        unsigned year  = (*date >> 9) & 0x7f;

        /* simplification: ignore leap years */
        if (++day > daysinmonth[month-1])
        {
            day = 1;
            if (++month > 12)
            {
                month = 1;
                year++;
            }
        }

        *date = (year << 9) | (month << 5) | day;
    }
}
#endif /* CONFIG_RTC */

static int write_longname(struct filestr *dirstr,
                          struct fileinfo *info,
                          const unsigned char *name,
                          unsigned long ucslen,
                          const unsigned char *shortname,
                          union raw_dirent *srcent,
                          uint8_t attr,
                          unsigned int flags)
{
    DEBUGF("%s:info=%p:fstclus=%lu:#de=%u:de0=%u:name='%s'\n", __func__,
           info, info->firstcluster, info->numentries,
           info->direntry - info->numentries + 1, name);

    int rc;
    union raw_dirent *ent;

    uint16_t date = 0, time = 0, tenth = 0;
    fat_time(&date, &time, &tenth);
    time = htole16(time);
    date = htole16(date);

    /* shortname checksum saved in each longname entry */
    uint8_t chksum = shortname_checksum(shortname);

    /* we need to convert the name first since the entries are written in
       reverse order */
    unsigned long ucspadlen = ALIGN_UP(ucslen, FATLONG_NAME_CHARS);
    uint16_t ucsname[ucspadlen];

    for (unsigned long i = 0; i < ucspadlen; i++)
    {
        if (i < ucslen)
            name = utf8decode(name, &ucsname[i]);
        else if (i == ucslen)
            ucsname[i] = 0x0000; /* name doesn't fill last block */
        else /* i > ucslen */
            ucsname[i] = 0xffff; /* pad-out to end */
    }

    const unsigned int longentries = info->numentries - 1;
    const unsigned int firstentry  = info->direntry - longentries;

    /* longame entries */
    for (unsigned int i = 0; i < longentries; i++)
    {
        ent = cache_direntry(dirstr, firstentry + i);
        if (!ent)
            FAT_ERROR(-2);

        /* verify this entry is free */
        if (ent->name[0] && ent->name[0] != 0xe5)
        {
            panicf("Dir entry %d in sector %x is not free! "
                   "%02x %02x %02x %02x",
                   i + firstentry, (unsigned)dirstr->lastsector,
                   (unsigned)ent->data[0], (unsigned)ent->data[1],
                   (unsigned)ent->data[2], (unsigned)ent->data[3]);
        }

        memset(ent->data, 0, DIR_ENTRY_SIZE);

        unsigned int ord = longentries - i;

        ent->ldir_ord = ord | (i == 0 ? FATLONG_ORD_F_LAST : 0);
        ent->ldir_attr = ATTR_LONG_NAME;
        ent->ldir_chksum = chksum;

        /* set name */
        uint16_t *ucsptr = &ucsname[(ord - 1) * FATLONG_NAME_CHARS];
        for (unsigned j = longent_char_first(); j; j = longent_char_next(j))
        {
            uint16_t ucs = *ucsptr++;
            INT162BYTES(ent->data, j, ucs);
        }

        iobuf_release_dirty(&dirstr->iob);
        DEBUGF("Longname entry %u\n", ord);
    }

    /* shortname entry */
    DEBUGF("Shortname '%s'\n", shortname);

    ent = cache_direntry(dirstr, info->direntry);
    if (!ent)
        FAT_ERROR(-2);

    if (srcent && (flags & DIRENT_TEMPL))
    {
        /* srcent points to short entry template */
        *ent = *srcent;
    }
    else
    {
        /* make our own short entry */
        memset(ent->data, 0, DIR_ENTRY_SIZE);
        ent->attr = attr;
    }

    /* short name may change even if just moving */
    memcpy(ent->name, shortname, 11);
    dosent_set_fstclus(info->firstcluster, ent);

    if (!(flags & DIRENT_TEMPL_CRT))
    {
        ent->crttimetenth = tenth;
        ent->crttime      = time;
        ent->crtdate      = date;
    }

    if (!(flags & DIRENT_TEMPL_WRT))
    {
        ent->wrttime = time;
        ent->wrtdate = date;
    }

    if (!(flags & DIRENT_TEMPL_ACC))
        ent->lstaccdate = date;

    if (srcent && (flags & DIRENT_RETURN))
        *srcent = *ent; /* caller wants */

    iobuf_release_dirty(&dirstr->iob);

    rc = 0;
fat_error:
    return rc;
}

static int add_dir_entry(struct filestr *dirstr,
                         struct fileinfo *info,
                         const char *name,
                         union raw_dirent *srcent,
                         uint8_t attr,
                         unsigned int flags)
{
    DEBUGF("%s:name='%s':fstclus=%ld\n", __func__, name, info->firstcluster);

    int rc, rc2 = 0;

    struct bpb *bpb = dirstr->infop->bpb;

    unsigned char basisname[11], shortname[11];
    int n;
    int entries_needed;
    unsigned long ucslen = 0;

    if (is_dotdir_name(name) && (attr & ATTR_DIRECTORY))
    {
        /* The "." and ".." directory entries must not be long names */
        int dots = strlcpy(shortname, name, 11);
        memset(&shortname[dots], ' ', 11 - dots);
        entries_needed = 1;
    }
    else
    {
        rc2 = check_longname(name);
        if (rc2 < 0)
            FAT_ERROR(-1); /* filename is invalid */

        create_dos_name(basisname, name, &n);
        randomize_dos_name(shortname, basisname, &n);

        /* one dir entry needed for every 13 characters of filename,
           plus one entry for the short name */
        ucslen = utf8length(name);
        if (ucslen > 255)
        {
            rc2 = -ENAMETOOLONG*10;
            FAT_ERROR(-2);
        }

        entries_needed = (ucslen + FATLONG_NAME_CHARS - 1)
                            / FATLONG_NAME_CHARS + 1;
    }

    int entry = 0, entries_found = 0, firstentry = -1;
    const int entperclus = DIR_ENTRIES_PER_SECTOR * bpb->secperclus;

    /* step 1: search for a sufficiently-long run of free entries and check
               for duplicate shortname */
    for (bool done = false; !done;)
    {
        union raw_dirent *ent = cache_direntry(dirstr, entry);
        if (!ent)
        {
            if (dirstr->eof)
            {
                DEBUGF("End of dir (entry %d)\n", entry);
                break;
            }

            DEBUGF("Couldn't read dir (entry %d)\n", entry);
            FAT_ERROR(-3);
        }

        switch (ent->name[0])
        {
        case 0:    /* all remaining entries in cluster are free */
            DEBUGF("Found end of dir %d\n", entry);
            int found = entperclus - (entry % entperclus);
            entries_found += found;
            entry += found; /* move entry passed end of cluster */
            done = true;
            break;

        case 0xe5: /* individual free entry */
            entries_found++;
            entry++;
            DEBUGF("Found free entry %d (%d/%d)\n",
                   entry, entries_found, entries_needed);
            break;

        default:   /* occupied */
            entries_found = 0;
            entry++;

            if ((ent->ldir_attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME)
                break; /* ignore long name entry */

            /* check that our intended shortname doesn't already exist */
            if (!strncmp(shortname, ent->name, 11))
            {
                /* shortname exists already, make a new one */
                DEBUGF("Duplicate shortname '%.11s'", shortname);
                randomize_dos_name(shortname, basisname, &n);
                DEBUGF(", changing to '%.11s'\n", shortname);

                /* name has changed, we need to restart search */
                entry = 0;
                firstentry = -1;
            }
            break;
        }

        iobuf_release(&dirstr->iob);

        if (firstentry < 0 && entries_found >= entries_needed)
        {
            /* found adequate space; point to initial free entry */
            firstentry = entry - entries_found;
        }
    }

    /* step 2: extend the dir if necessary */
    if (firstentry < 0)
    {
        DEBUGF("Adding new cluster(s) to dir\n");

        if (entry + entries_needed - entries_found > MAX_DIRENTRIES)
        {
            /* FAT specification allows no more than 65536 entries (2MB)
               per directory */
            DEBUGF("Directory would be too large.\n");
            FAT_ERROR(-4);
        }

        while (entries_found < entries_needed)
        {
            file_size_t nbyte = bpb->clussize;
            rc2 = fat_extend_file(dirstr, 0, &nbyte);
            if (rc2 < 0)
                FAT_ERROR(-5);

            entries_found += entperclus;
            entry += entperclus;
        }

        firstentry = entry - entries_found;
    }

    /* remember the parent directory entry information */
    info->dircluster = dirstr->infop->firstcluster;
    info->direntry   = firstentry + entries_needed - 1;
    info->numentries = entries_needed;

    /* step 3: add entry */
    DEBUGF("Adding longname to entry %u\n", firstentry);
    rc2 = write_longname(dirstr, info, name, ucslen,
                         shortname, srcent, attr, flags);
    if (rc2 < 0)
        FAT_ERROR(-6);

    DEBUGF("Added new dir entry %u; using %u entries\n",
           info->direntry, info->numentries);

    rc = 0;
fat_error:
    return FAT_ERROR_RC(rc, rc2);
}

static int update_short_entry(struct fileinfo *info,
                              struct fileentry *entry)
{
    DEBUGF("%s:cluster=%lu:entry=%u:size=%lu\n",
           __func__, info->firstcluster, info->direntry,
           (unsigned long)info->size);

    if (!info->dircluster)
    {
        entry->shortname[0] = '\0';
        entry->attr         = (info->flags & FO_DIRECTORY) ?
                                    ATTR_DIRECTORY : 0;
        entry->crttime      = 0;
        entry->crtdate      = 0;
        entry->lstaccdate   = 0;
        entry->wrttime      = 0;
        entry->wrtdate      = 0;
        entry->filesize     = info->size;
        entry->firstcluster = info->firstcluster;
        entry->direntry     = FAT_DIRSCAN_RW_VAL;
        entry->numentries   = 0;
        return 0;
    }

    int rc;

    /* open the parent directory */
    struct fileinfo dirinfo;
    struct filestr dirstr;
    fat_fileinfo_init(info->bpb, info->dircluster, &dirinfo, &dirstr);

    /* retrieve the dos entry */
    union raw_dirent *ent = cache_direntry(&dirstr, info->direntry);
    if (!ent)
        FAT_ERROR(-1);

    if (!ent->name[0] || ent->name[0] == 0xe5)
    {
        panicf("Updating free direntry %u:%02x\n",
               info->direntry, (unsigned int)ent->name[0]);
    }

    /* basic file data */
    dosent_set_fstclus(info->firstcluster, ent);
    ent->filesize = htole32((uint32_t)info->size);

    /* time and date info */
#if CONFIG_RTC
    uint16_t time = 0;
    uint16_t date = 0;
#else
    /* get old time to increment from */
    uint16_t time = letoh16(ent->wrttime);
    uint16_t date = letoh16(ent->wrtdate);
#endif
    fat_time(&date, &time, NULL);
    date = htole16(date);
    time = htole16(time);

    ent->wrttime    = time;
    ent->wrtdate    = date;
    ent->lstaccdate = date;

    fill_fileentry(ent, info, entry);

    iobuf_release_dirty(&dirstr.iob);

    rc = 0;
fat_error:
    return rc;
}

static int free_direntries(struct fileinfo *info)
{
    int rc = 0;

    /* open the parent directory */
    struct fileinfo dirinfo;
    struct filestr dirstr;
    fat_fileinfo_init(info->bpb, info->dircluster, &dirinfo, &dirstr);

    for (unsigned int entries = info->numentries,
                      entry = info->direntry - entries + 1;
         entries; entries--, entry++)
    {
        DEBUGF("Clearing dir entry %u (%u/%u)\n",
               entry, entries - info->numentries + 1, info->numentries);

        union raw_dirent *ent = cache_direntry(&dirstr, entry);
        if (!ent)
        {
            if (entries == info->numentries)
                FAT_ERROR(-1); /* nothing at all freed */

            /* longname already destroyed; revert to shortname */
            info->numentries = 1;
            FAT_ERROR(RC);
        }

        ent->name[0] = 0xe5;

        iobuf_release_dirty(&dirstr.iob);
    }

    /* directory entry info is now gone */
    info->dircluster = 0;
    info->direntry   = FAT_DIRSCAN_RW_VAL;
    info->numentries = 0;

    rc = 1;
fat_error:
    return rc;
}

static int fat_create_file(struct fileinfo *dirinfo,
                           uint8_t attr,
                           struct fileentry *entry)
{
    DEBUGF("%s:di=%p:attr=%02X:name='%s'\n", __func__, dirinfo,
           attr, entry->name);

    int rc, rc2 = 0;

    /* open the parent directory */
    struct filestr dirstr;
    fat_filestr_init(dirinfo, &dirstr);

    /* open a proto file */
    struct fileinfo info;
    fat_fileinfo_init(dirinfo->bpb, 0, &info, NULL);

    unsigned int addflags = DIRENT_RETURN;
    union raw_dirent dosent;

    if (attr & ATTR_DIRECTORY)
    {
        struct filestr str;
        fat_filestr_init(&info, &str);

        /* create the first cluster */
        file_size_t nbyte = info.bpb->clussize;
        rc2 = fat_extend_file(&str, 0, &nbyte);
        if (rc2 < 0)
            FAT_ERROR(-1);

        struct fileinfo dummyfile;

        /* add the "." entry */
        fat_fileinfo_init(info.bpb, info.firstcluster, &dummyfile, NULL);

        /* this returns the short entry template for the remaining entries */
        rc2 = add_dir_entry(&str, &dummyfile, ".", &dosent,
                            ATTR_DIRECTORY, DIRENT_RETURN);
        if (rc2 < 0)
            FAT_ERROR(-2);

        /* and the ".." entry */
        /* the root cluster is cluster 0 in the ".." entry */
        long clust = dirinfo->firstcluster;
        if (clust == info.bpb->rootclus)
            clust = 0;

        fat_fileinfo_init(info.bpb, clust, &dummyfile, NULL);

        rc2 = add_dir_entry(&str, &dummyfile, "..", &dosent,
                            ATTR_DIRECTORY, DIRENT_TEMPL_TIMES);
        if (rc2 < 0)
            FAT_ERROR(-3);

        addflags |= DIRENT_TEMPL_TIMES;
    }

    /* lastly, add the entry in the parent directory */
    rc2 = add_dir_entry(&dirstr, &info, entry->name, &dosent, attr,
                        addflags);
    if (rc2 < 0)
        FAT_ERROR(-4);

    /* entry->name is provided by caller */
    fill_fileentry(&dosent, &info, entry);

    rc = 0;
fat_error:
    if (rc < 0)
        free_cluster_chain(info.bpb, info.firstcluster, &dirstr.fatiob);

    cache_fileop_commit(dirinfo->bpb);
    return FAT_ERROR_RC(rc, rc2);
}

static int fat_rename(struct fileinfo *dirinfo,
                      struct fileinfo *info,
                      struct fileentry *entry)
{
    int rc, rc2 = 0;

    if (!info->dircluster)
    {
        /* file was removed but is still open */
        DEBUGF("File has no dir cluster!\n");
        FAT_ERROR(-1);
    }

#ifdef HAVE_MULTIVOLUME
    /* rename only works on the same volume */
    if (info->bpb != dirinfo->bpb)
    {
        DEBUGF("No rename across volumes!\n");
        FAT_ERROR(-2);
    }
#endif /* HAVE_MULTIVOLUME */

    /* root directories can't be renamed */
    if (info->firstcluster == info->bpb->rootclus)
    {
        DEBUGF("Trying to rename root of volume %d\n",
               (int)info->bpb->volume);
        FAT_ERROR(-3);
    }

    /* use copy so as not to disturb old info if anything fails */
    struct fileinfo newinfo = *info;
    struct filestr  str;
    union raw_dirent dosent;

    {
        /* fetch a copy of the existing short entry as a template */
        struct fileinfo olddir;
        fat_fileinfo_init(info->bpb, info->dircluster, &olddir, &str);

        union raw_dirent *ent = cache_direntry(&str, info->direntry);
        if (!ent)
            FAT_ERROR(-4);

        dosent = *ent;

        iobuf_release(&str.iob);
    }

    /* create new name in new parent directory */
    fat_filestr_init(dirinfo, &str);
    rc2 = add_dir_entry(&str, &newinfo, entry->name, &dosent, 0,
                        DIRENT_TEMPL_CRT | DIRENT_TEMPL_WRT |
                        DIRENT_RETURN);
    if (rc2 < 0)
        FAT_ERROR(-5);

    /* if renaming a directory and it was a move, update the '..' entry to
       keep it pointing to its parent directory */
    if ((dosent.attr & ATTR_DIRECTORY) &&
            newinfo.dircluster != info->dircluster)
    {
        /* open the dir that was renamed */
        fat_filestr_init(&newinfo, &str);

        /* obtain dot-dot directory entry */
        union raw_dirent *ent = cache_direntry(&str, 1);
        if (!ent)
            FAT_ERROR(-6);

        if (memcmp("..         ", ent->name, 11))
        {
            /* .. entry must be second entry according to FAT spec (p.29) */
            DEBUGF("Second dir entry is not double-dot!\n");
            iobuf_release(&str.iob);
            FAT_ERROR(-7);
        }

        /* parent cluster is 0 if parent dir is the root - FAT spec (p.29) */
        long clust = dirinfo->firstcluster;
        if (clust == newinfo.bpb->rootclus)
            clust = 0;

        dosent_set_fstclus(clust, ent);
        iobuf_release_dirty(&str.iob);
    }

    /* remove old name */
    rc2 = free_direntries(info);
    if (rc2 <= 0)
        FAT_ERROR(-8);

    /* update the old entry with the new directory entry info */
    info->dircluster  = newinfo.dircluster;
    info->direntry    = newinfo.direntry;
    info->numentries  = newinfo.numentries;

    fill_fileentry(&dosent, &newinfo, entry);

    rc = 0;
fat_error:
    /* free the new entry if it was created but later ops failed */
    if (rc && rc2 && fat_fileop_compare_fileinfo(&newinfo, info) != CMPFI_EQ)
        free_direntries(&newinfo);

    cache_fileop_commit(info->bpb);
    return FAT_ERROR_RC(rc, rc2);
}

static int fat_sync_file(struct fileinfo *info,
                         struct fileentry *entry)
{
    DEBUGF("%s:size=%lu\n", __func__, info->size);

    int rc, rc2 = 0;

    file_size_t size = info->size;

    if (!size && info->firstcluster)
    {
        /* empty file */
        struct iobuf_handle fatiob;
        iobuf_init(&fatiob);

        rc2 = update_fat_entry(info->bpb, info->firstcluster, 0,
                               &fatiob);
        if (rc2 < 0)
            FAT_ERROR(-1);

        info->firstcluster = 0;
    }

    rc2 = update_short_entry(info, entry);
    if (rc2 < 0)
        FAT_ERROR(-4);

    rc = 0;
fat_error:
    cache_fileop_commit(info->bpb);
    return FAT_ERROR_RC(rc, rc2);
}


/** File Ops **/

enum cmpfi_result fat_fileop_compare_fileinfo(const struct fileinfo *info1,
                                              const struct fileinfo *info2)
{
    /* if there's no dircluster, the file has been removed; only an entry
       within the same valid parent directory has any sorting value */
#ifdef HAVE_MULTIVOLUME
    if (!(info1->bpb && info2->bpb))
        return CMPFI_BADF;

    if (info1->bpb != info2->bpb)
        return CMPFI_XDEV;
#endif /* HAVE_MULTIVOLUME */

    if (!(info1->dircluster && info2->dircluster))
        return CMPFI_BADF;

    if (info1->dircluster != info2->dircluster)
        return CMPFI_XDIR;

    if (info1->direntry == info2->direntry)
        return CMPFI_EQ;
    else if (info1->direntry < info2->direntry)
        return CMPFI_LT;
    else /* info1->direntry > info2->direntry */
        return CMPFI_GT;
}

int fat_fileop_open_volume(struct bpb *bpb,
                           struct fileinfo *info)
{
    if (!bpb_ismounted_internal(bpb))
        return -ENXIO;

    fat_fileinfo_init(bpb, bpb->rootclus, info, NULL);
    info->dircluster = bpb->rootclus;

    return 0;
}

int fat_fileop_open(struct fileinfo *dirinfo,
                    struct fileentry *entry,
                    struct fileinfo *info)
{
    /* entry should be obtained by get_fileentry or create */
    fileinfo_hdr_init(dirinfo->bpb, entry->filesize, info);
    info->firstcluster = entry->firstcluster;
    info->dircluster   = dirinfo->firstcluster;
    info->direntry     = entry->direntry;
    info->numentries   = entry->numentries;

    return 0;
}

int fat_fileop_create(struct fileinfo *dirinfo,
                      unsigned int callflags,
                      struct fileentry *entry)
{
    uint8_t attr = (callflags & FO_DIRECTORY) ?
                            ATTR_DIRECTORY : ATTR_ARCHIVE;

    int rc = fat_create_file(dirinfo, attr, entry);
    return FAT_ERROR_RC_ERRNO(rc);
}

int fat_fileop_close(struct fileinfo *info)
{
    int rc;

    if (!info->dircluster && info->firstcluster &&
        info->firstcluster != info->bpb->rootclus)
    {
        /* mark all clusters in the chain as free */
        DEBUGF("Removing cluster chain:fstclus=%ld\n", info->firstcluster);

        struct iobuf_handle fatiob;
        iobuf_init(&fatiob);

        rc = free_cluster_chain(info->bpb, info->firstcluster, &fatiob);
        if (rc < 0)
            FILE_ERROR_RC(RC);

        /* at least the first cluster was freed */
        info->firstcluster = 0;

        cache_fileop_commit(info->bpb);
    }

    rc = 0;
file_error:
    return FAT_ERROR_RC_ERRNO(rc);
}

int fat_fileop_remove(struct fileinfo *info)
{
    int rc;

    if (info->firstcluster == info->bpb->rootclus)
    {
        DEBUGF("Trying to remove root:vol=%d\n", (int)info->bpb->volume);
        FILE_ERROR_RC(-EIO*10);
    }

    if (info->dircluster)
    {
        /* free everything in the parent directory */
        DEBUGF("Removing dir entries:dclus=%ld\n", info->dircluster);
        rc = free_direntries(info);
        if (rc <= 0)
            FILE_ERROR_RC(RC);
    }

    rc = 0;
file_error:
    return FAT_ERROR_RC_ERRNO(rc);
}

int fat_fileop_sync(struct fileinfo *info,
                    struct fileentry *entry)
{
    int rc = fat_sync_file(info, entry);
    return FAT_ERROR_RC_ERRNO(rc);
}

int fat_fileop_rename(struct fileinfo *dirinfo,
                      struct fileinfo *info,
                      struct fileentry *entry)
{
    int rc = fat_rename(dirinfo, info, entry);
    return FAT_ERROR_RC_ERRNO(rc);
}

int fat_fileop_open_filestr(struct fileinfo *info,
                            struct filestr *str)
{
    fat_filestr_init(info, str);
    return 0;
}

int fat_fileop_ftruncate(struct filestr *str,
                         file_size_t truncsize)
{
    int rc;

    struct fileinfo *info = str->infop;
    file_size_t size = info->size;

    if (size == truncsize)
        return 0;

    if (truncsize > FAT_FILESIZE_MAX)
        return -EFBIG;

    rc = fat_seek_offset(str, truncsize < size ? truncsize : size);
    if (rc < 0)
        FILE_ERROR_RC(RC);

    if (truncsize < size)
    {
        /* free clusters */
        rc = fat_shorten_file(str, size, &truncsize);
    }
    else /* truncsize > size */
    {
        /* allocate clusters */
        rc = fat_extend_file(str, size, &truncsize);
        if (rc < 0 && fat_seek_offset(str, size) >= 0)
        {
            /* leave file unaffected if possible */
            fat_shorten_file(str, truncsize, &size);
            truncsize = size;
        }
    }

    info->size = truncsize;

    if (rc < 0)
        FILE_ERROR_RC(RC);

    rc = 0;
file_error:
    return FAT_ERROR_RC_ERRNO(rc);
}

/* set the file pointer */
off_t fat_fileop_lseek(struct filestr *str,
                       off_t offset,
                       int whence)
{
    return file_fileop_lseek(str, offset, whence, FAT_FILESIZE_MAX);
}

/* lead byte handler for fat_fileop_readwrite */
static FORCE_INLINE ssize_t readwrite_lead(struct filestr *str,
                                           struct fileinfo *info,
                                           void *buf,
                                           ssize_t secoffs,
                                           size_t nbyte,
                                           bool write)
{
    void *cbuf = cache_sector_buffer(info->bpb, str->lastsector,
                                     &str->iob, SECPRIO_DATA, true);
    if (!cbuf)
        return -1;

    cbuf += secoffs;

    if (write)
    {
        memcpy(cbuf, buf, nbyte);
        iobuf_release_dirty(&str->iob);
    }
    else
    {
        memcpy(buf, cbuf, nbyte);
        iobuf_release(&str->iob);
    }

    return nbyte;
}

/* tail byte handler for fat_fileop_readwrite */
static FORCE_INLINE ssize_t readwrite_tail(struct filestr *str,
                                           struct fileinfo *info,
                                           bool fill,
                                           void *buf,
                                           size_t nbyte,
                                           bool write)
{
    /* last-access has to be moved to this sector or it may be necessary to
       allocate a new cluster if this is a write */
    int rc = fat_readwrite(str, &(unsigned long){ 1 }, NULL, write);
    if (rc < 0)
        return rc;

    void *cbuf = cache_sector_buffer(info->bpb, str->lastsector,
                                     &str->iob, SECPRIO_DATA, fill);
    if (!cbuf)
        return -1;

    if (write)
    {
        memcpy(cbuf, buf, nbyte);
        iobuf_release_dirty(&str->iob);
    }
    else
    {
        memcpy(buf, cbuf, nbyte);
        iobuf_release(&str->iob);
    }

    return nbyte;
}

/* read or write bytes on or from a file */
ssize_t fat_fileop_readwrite(struct filestr *str,
                             void *buf,
                             size_t nbyte,
                             bool write)
{
    DEBUGF("%s:str=%p:buf=%p:nby=%lu:wr=%d\n", __func__, str,
           buf, (unsigned long)nbyte, (int)write);

    if (nbyte == 0)
        return 0;

    ssize_t rc = 0;

    struct fileinfo *info = str->infop;
    file_size_t size   = info->size;
    file_size_t offset = str->offset;
    file_size_t filerem;

    if (write)
    {
        /* if opened in append mode, move pointer to end */
        if (str->flags & FD_APPEND)
            str->offset = offset = size;

        if (offset > size)
        {
            /* zero-fill the gap */
            rc = fat_seek_offset(str, size);
            if (rc < 0)
                FILE_ERROR_RC(RC);

            file_size_t newsize = offset;
            rc = fat_extend_file(str, size, &newsize);

            info->size = size = newsize;

            if (rc < 0)
                FILE_ERROR_RC(RC);
        }

        filerem = FAT_FILESIZE_MAX - offset;
        if (!filerem)
            FILE_ERROR_RC(-EFBIG*10); /* would get too large */
    }
    else
    {
        if (info->flags & FO_DIRECTORY)
            size = MAX_DIRECTORY_SIZE;

        filerem = size - MIN(offset, size);
    }

    if (filerem < nbyte)
        nbyte = filerem;

    if (nbyte)
    {
        unsigned long secnum = offset / SECTOR_SIZE;
        unsigned long secoffs = offset % SECTOR_SIZE;
        if (secoffs)
            secnum++;

        if (str_last_secnum(str) + 1 != secnum)
        {
            /* get on the correct sector */
            rc = fat_seek(str, secnum);
            if (rc < 0)
                FILE_ERROR_RC(RC);
        }

        /* any lead bytes? */
        if (secoffs)
        {
            rc = MIN(nbyte, SECTOR_SIZE - secoffs);
            rc = readwrite_lead(str, info, buf, secoffs, rc, write);
            if (rc < 0)
                FILE_ERROR_RC(RC);

            buf    += rc;
            nbyte  -= rc;
            offset += rc;
        }
    }

    /* read/write whole sectors */
    if (nbyte >= SECTOR_SIZE)
    {
        unsigned long count = nbyte / SECTOR_SIZE;
        unsigned long transferred = count;
        rc = fat_readwrite(str, &transferred, buf, write);

        size_t bytes = transferred * SECTOR_SIZE;
        buf    += bytes;
        nbyte  -= bytes;
        offset += bytes;

        if (rc < 0 || transferred < count)
        {
            /* error or eof */
            if (rc < 0)
            {
                DEBUGF("I/O error %sing %lu sectors\n",
                       write ? "writ" : "read", count);
            }

            FILE_ERROR_RC(RC);
        }
    }

    /* any tail bytes? */
    if (nbyte)
    {
        rc = readwrite_tail(str, info, !write || offset < size, buf,
                            nbyte, write);
        if (rc < 0)
            FILE_ERROR_RC(RC);

        buf    += rc;
        offset += rc;
    }

file_error:
    if (rc == -ENOSPC*10)
        DEBUGF("No space left on device\n");

    /* error or not, update the file offset and size if anything was
       transferred and return the amount */
    if (offset > str->offset)
    {
        ssize_t done = offset - str->offset;

        str->offset = offset;

        DEBUGF("file offset=%lu\n", offset);

        /* adjust file size to length written */
        if (write)
        {
            if (offset > size)
                info->size = offset;

            /* a new cluster could have been allocated but the storage
               transfer failed; try to clean it up */
            if (rc < 0 && offset >= size && fat_seek_offset(str, offset) >= 0)
                fat_shorten_file(str, offset, &size);
        }

        rc = done;
    }

    return FAT_ERROR_RC_ERRNO(rc);
}

int fat_fileop_compare_name(struct fileinfo *dirinfo,
                            const char *name1,
                            const char *name2,
                            size_t maxlen)
{
    return strncasecmp(name1, name2, maxlen);
    (void)dirinfo;
}

int fat_fileop_readdir(struct filestr *dirstr,
                       struct dirscan *scan,
                       struct fileentry *entry)
{
    int rc = FSI_S_RDDIR_EOD;

    /* long file names are stored in special entries; each entry holds up to
       13 UTF-16 characters' thus, UTF-8 converted names can be max 255 chars
       (1020 bytes) long, not including the trailing '\0'. */
    struct fatlong_parse_state lnparse;
    fatlong_parse_start(&lnparse, entry);

    unsigned int direntry = scan->direntry;
    unsigned long last_secnum = MAX_DIRENTRIES / DIR_ENTRIES_PER_SECTOR;
    union raw_dirent *buf = NULL;

    while (1)
    {
        if (++direntry >= MAX_DIRENTRIES)
        {
            DEBUGF("%s:Dir is too large:entry=%u\n", __func__, direntry);
            FILE_ERROR_RC(-EFBIG*10);
        }

        unsigned long secnum = direntry / DIR_ENTRIES_PER_SECTOR;
        if (last_secnum != secnum)
        {
            if (buf)
                iobuf_release(&dirstr->iob);

            buf = cache_sector_num(dirstr, secnum, SECPRIO_DATA);
            last_secnum = secnum;
        }

        if (!buf)
        {
            if (dirstr->eof)
                break;

            DEBUGF("%s:Couldn't read dir\n", __func__);
            FILE_ERROR_RC(-EIO*10);
        }

        union raw_dirent *ent = &buf[direntry % DIR_ENTRIES_PER_SECTOR];

        if (ent->name[0] == 0)
            break;    /* last entry */

        if (ent->name[0] == 0xe5)
            continue; /* free entry */

        if (IS_LDIR_ATTR(ent->ldir_attr))
        {
            /* LFN entry */
            if (UNLIKELY(!fatlong_parse_entry(&lnparse, ent, entry)))
            {
                /* resync so we don't return just the short name if what we
                   landed in the middle of is valid (directory changes
                   between calls likely created the situation; ignoring this
                   case can be harmful elsewhere and is destructive to the
                   entry series itself) */
                while (--direntry != FAT_DIRSCAN_RW_VAL) /* at beginning? */
                {
                    iobuf_release(&dirstr->iob);
                    buf = ent = cache_direntry(dirstr, direntry);
                    if (!ent)
                        break;

                    /* name[0] == 0 shouldn't happen here but... */
                    if (ent->name[0] == 0 || ent->name[0] == 0xe5 ||
                        !IS_LDIR_ATTR(ent->ldir_attr))
                        break;
                }

                /* retry it once from the new position */
            }
        }
        else if (!IS_VOL_ID_ATTR(ent->attr)) /* ignore volume id entry */
        {
            fatlong_parse_finish(&lnparse, ent, direntry, entry);
            rc = FSI_S_RDDIR_OK;
            break;
        }
    } /* end while */

file_error:
    if (buf)
        iobuf_release(&dirstr->iob);

    /* if error or eod, stay on last good position */
    if (!FSI_S_RDDIR_HAVEENT(rc))
        direntry--;

    scan->direntry = direntry;

    return FAT_ERROR_RC_ERRNO(rc);
}

int fat_fileop_rewinddir(struct filestr *dirstr,
                         struct dirscan *scan)
{
    /* rewind the directory to the beginning */
    scan->direntry = FAT_DIRSCAN_RW_VAL;
    return 0;
    (void)dirstr;
}

int fat_fileop_get_fileentry(struct fileinfo *dirinfo,
                             const char *name,
                             size_t namelen,
                             struct fileentry *entry)
{
    struct filestr dirstr;
    struct dirscan scan;

    fat_filestr_init(dirinfo, &dirstr);
    fat_fileop_rewinddir(&dirstr, &scan);

    int rc;

    while (1)
    {
        rc = fat_fileop_readdir(&dirstr, &scan, entry);
        if (!FSI_S_RDDIR_HAVEENT(rc))
            break;

        /* name == NULL means find anything that exists besides dotdirs */
        if (UNLIKELY(!name))
        {
            if (entry->namelen <= 2 && is_dotdir_name(entry->name))
                continue;

            break;
        }

        if (namelen != entry->namelen)
            continue;

        if (!fat_fileop_compare_name(dirinfo, name, entry->name, namelen))
            break;
    }

    return FAT_ERROR_RC_ERRNO(rc);
}

int fat_fileop_convert_entry(union direntry_cvt_t dst,
                             union direntry_cvt_t src,
                             unsigned int cvtflags)
{
    switch (cvtflags & (CVTENT_SRC_MASK|CVTENT_DST_MASK))
    {
    case CVTENT_SRC_FILEENTRY|CVTENT_DST_DIRENT:
        if (src.fileentry->numentries == 0)
        {
            *dst.dirent = (struct dirent){ .d_name = "" };
            break;
        }

        /* fileentry can have its name pointed to the source buffer */
        if (src.fileentry->name != dst.dirent->d_name)
        {
            memmove(dst.dirent->d_name, src.fileentry->name,
                    src.fileentry->namelen + 1);
        }

        dst.dirent->info.attr    = src.fileentry->attr;
        dst.dirent->info.size    = src.fileentry->filesize;
        dst.dirent->info.wrtdate = src.fileentry->wrtdate;
        dst.dirent->info.wrttime = src.fileentry->wrttime;
        break;

    case CVTENT_SRC_DIRENT|CVTENT_DST_DIRINFO:
        dst.dirinfo->attribute = src.dirent->info.attr;
        dst.dirinfo->size      = src.dirent->info.size;
        dst.dirinfo->mtime     = fattime_mktime(src.dirent->info.wrtdate,
                                                src.dirent->info.wrttime);
        break;

    default:
        return -EINVAL;
    }

    return 0;
}


/** Disk Ops **/

int fat_diskop_mount(struct bpb *bpb)
{
    int rc;

    if (bpb->mounted)
    {
        DEBUGF("%s:already mounted:vol=%d\n", __func__, (int)bpb->volume);
        FILE_ERROR_RC(-EBUSY*10);
    }

    rc = fat_mount(bpb);
    if (rc < 0)
        FILE_ERROR_RC(RC);

    __fsinfo_lock_init(bpb);
    iocache_mount_volume(bpb, bpb->numfats > 1 ? fat_mirror_wb_cb : NULL);

    /* calculate freecount if unset */
    if (bpb->fsinfo.freecount < 0)
        fat_recalc_free(bpb);

    DEBUGF("Freecount: %ld\n", bpb->fsinfo.freecount);
    DEBUGF("Nextfree: %ld\n", bpb->fsinfo.nextfree);
    DEBUGF("Cluster count: %ld\n", bpb->dataclusters);
    DEBUGF("Sectors per cluster: %lu\n", bpb->secperclus);
    DEBUGF("Cluster size: %lu\n", bpb->clussize);
    DEBUGF("FAT sectors: %lu\n", bpb->fatsz);

    rc = 0;
file_error:
    return FAT_ERROR_RC_ERRNO(rc);
}

int fat_diskop_unmount(struct bpb *bpb)
{
    int rc;

    if (!bpb->mounted)
    {
        DEBUGF("%s:not mounted:vol=%d\n", __func__, (int)bpb->volume);
        FILE_ERROR_RC(-EINVAL*10);
    }

    /* free the entries for this volume, try to flush them first */
    cache_commit(bpb);
    cache_discard(bpb);

    rc = 0;
file_error:
    return FAT_ERROR_RC_ERRNO(rc);
}


/** Volume Ops **/

size_t fat_volop_cluster_size(struct bpb *bpb)
{
    return bpb->clussize;
}

#ifdef MAX_LOG_SECTOR_SIZE
size_t fat_volop_get_bytes_per_sector(struct bpb *bpb)
{
    return bpb->bytspersec;
}
#endif /* MAX_LOG_SECTOR_SIZE */

void fat_volop_recalc_free(struct bpb *bpb)
{
    fat_recalc_free(bpb);
}

void fat_volop_size(struct bpb *bpb,
                    unsigned long *size,
                    unsigned long *free)
{
    unsigned long factor = bpb->clussize / 1024;
    if (size) *size = bpb->dataclusters * factor;
    if (free) *free = bpb->fsinfo.freecount * factor;
}

void fat_volop_flush(struct bpb *bpb)
{
    if (bpb_ismounted_internal(bpb))
        cache_commit(bpb);
}


/** Misc. **/

time_t fattime_mktime(uint16_t fatdate,
                      uint16_t fattime)
{
    /* this knows our mktime() only uses these struct tm fields */
    struct tm tm;
    tm.tm_sec  = ((fattime      ) & 0x1f) * 2;
    tm.tm_min  = ((fattime >>  5) & 0x3f);
    tm.tm_hour = ((fattime >> 11)       );
    tm.tm_mday = ((fatdate      ) & 0x1f);
    tm.tm_mon  = ((fatdate >>  5) & 0x0f) - 1;
    tm.tm_year = ((fatdate >>  9)       ) + 80;

    return mktime(&tm);
}
