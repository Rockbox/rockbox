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
#include "config.h"
#include "system.h"
#include "sys/types.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "fs_attr.h"
#include "pathfuncs.h"
#include "disk_cache.h"
#include "file_internal.h" /* for struct filestr_cache */
#include "storage.h"
#include "timefuncs.h"
#include "rbunicode.h"
#include "debug.h"
#include "panic.h"
/*#define LOGF_ENABLE*/
#include "logf.h"

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

#define CLUSTERS_PER_FAT_SECTOR     (SECTOR_SIZE / 4)
#define CLUSTERS_PER_FAT16_SECTOR   (SECTOR_SIZE / 2)
#define DIR_ENTRIES_PER_SECTOR      (SECTOR_SIZE / DIR_ENTRY_SIZE)
#define DIR_ENTRY_SIZE              32
#define FAT_BAD_MARK                0x0ffffff7
#define FAT_EOF_MARK                0x0ffffff8
#define FAT16_BAD_MARK              0xfff7
#define FAT16_EOF_MARK              0xfff8

struct fsinfo
{
    unsigned long freecount; /* last known free cluster count */
    unsigned long nextfree;  /* first cluster to start looking for free
                                clusters, or 0xffffffff for no hint */
};
/* fsinfo offsets */
#define FSINFO_FREECOUNT 488
#define FSINFO_NEXTFREE  492

#ifdef HAVE_FAT16SUPPORT
#define BPB_FN_SET16(bpb, fn)      (bpb)->fn##__ = fn##16
#define BPB_FN_SET32(bpb, fn)      (bpb)->fn##__ = fn##32
#define BPB_FN_DECL(fn, args...)   (*fn##__)(struct bpb *bpb , ##args)
#define BPB_CALL(fn, bpb, args...) ((bpb)->fn##__(bpb , ##args))

#define get_next_cluster(bpb, cluster) \
    BPB_CALL(get_next_cluster, (bpb), (cluster))
#define find_free_cluster(bpb, startcluster) \
    BPB_CALL(find_free_cluster, (bpb), (startcluster))
#define update_fat_entry(bpb, entry, value) \
    BPB_CALL(update_fat_entry, (bpb), (entry), (value))
#define fat_recalc_free_internal(bpb) \
    BPB_CALL(fat_recalc_free_internal, (bpb))
#else  /* !HAVE_FAT16SUPPORT */
#define get_next_cluster            get_next_cluster32
#define find_free_cluster           find_free_cluster32
#define update_fat_entry            update_fat_entry32
#define fat_recalc_free_internal    fat_recalc_free_internal32
#endif /* HAVE_FAT16SUPPORT */
struct bpb;
static void update_fsinfo32(struct bpb *fat_bpb);

/* Note: This struct doesn't hold the raw values after mounting if
 * bpb_bytspersec isn't 512. All sector counts are normalized to 512 byte
 * physical sectors. */
static struct bpb
{
    unsigned long bpb_bytspersec; /* Bytes per sector, typically 512 */
    unsigned long bpb_secperclus; /* Sectors per cluster */
    unsigned long bpb_rsvdseccnt; /* Number of reserved sectors */
    unsigned long bpb_totsec16;   /* Number of sectors on volume (old 16-bit) */
    uint8_t       bpb_numfats;    /* Number of FAT structures, typically 2 */
    uint8_t       bpb_media;      /* Media type (typically 0xf0 or 0xf8) */
    uint16_t      bpb_fatsz16;    /* Number of used sectors per FAT structure */
    unsigned long bpb_totsec32;   /* Number of sectors on the volume
                                     (new 32-bit) */
    uint16_t      last_word;      /* 0xAA55 */
    long          bpb_rootclus;

    /**** FAT32 specific *****/
    unsigned long bpb_fatsz32;
    unsigned long bpb_fsinfo;

    /* variables for internal use */
    unsigned long fatsize;
    unsigned long totalsectors;
    unsigned long rootdirsector;
    unsigned long firstdatasector;
    unsigned long startsector;
    unsigned long dataclusters;
    unsigned long fatrgnstart;
    unsigned long fatrgnend;
    struct fsinfo fsinfo;
#ifdef HAVE_FAT16SUPPORT
    unsigned int bpb_rootentcnt;    /* Number of dir entries in the root */
    /* internals for FAT16 support */
    unsigned long rootdirsectornum; /* sector offset of root dir relative to start
                                     * of first pseudo cluster */
#endif /* HAVE_FAT16SUPPORT */

    /** Additional information kept for each volume **/
#ifdef HAVE_FAT16SUPPORT
    uint8_t is_fat16; /* true if we mounted a FAT16 partition, false if FAT32 */
#endif
#ifdef HAVE_MULTIDRIVE
    uint8_t drive;    /* on which physical device is this located */
#endif
#ifdef HAVE_MULTIVOLUME
    uint8_t volume;   /* on which volume is this located (shortcut) */
#endif
    uint8_t mounted;  /* true if volume is mounted, false otherwise */
#ifdef HAVE_FAT16SUPPORT
    /* some functions are different for different FAT types */
    long BPB_FN_DECL(get_next_cluster, long);
    long BPB_FN_DECL(find_free_cluster, long);
    int  BPB_FN_DECL(update_fat_entry, unsigned long, unsigned long);
    void BPB_FN_DECL(fat_recalc_free_internal);
#endif /* HAVE_FAT16SUPPORT */

} fat_bpbs[NUM_VOLUMES]; /* mounted partition info */

#define IS_FAT_SECTOR(bpb, sector) \
    (!((sector) >= (bpb)->fatrgnend || (sector) < (bpb)->fatrgnstart))

/* set error code and jump to routine exit */
#define FAT_ERROR(_rc) \
    ({  __builtin_constant_p(_rc) ?            \
            ({ if (_rc != RC) rc = (_rc); }) : \
            ({ rc = (_rc); });                 \
        goto fat_error; })

#define FAT_BPB(volume) \
    ({  struct bpb * _bpb = &fat_bpbs[IF_MV_VOL(volume)]; \
        if (!_bpb->mounted)                          \
        {                                            \
            DEBUGF("%s() - volume %d not mounted\n", \
                   __func__, IF_MV_VOL(volume));     \
            _bpb = NULL;                             \
        }                                            \
        _bpb; })

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

static void cache_commit(struct bpb *fat_bpb)
{
    dc_lock_cache();
#ifdef HAVE_FAT16SUPPORT
    if (!fat_bpb->is_fat16)
#endif
        update_fsinfo32(fat_bpb);
    dc_commit_all(IF_MV(fat_bpb->volume));
    dc_unlock_cache();
}

static void cache_discard(IF_MV_NONVOID(struct bpb *fat_bpb))
{
    dc_lock_cache();
    dc_discard_all(IF_MV(fat_bpb->volume));
    dc_unlock_cache();
}

/* caches a FAT or data area sector */
static void * cache_sector(struct bpb *fat_bpb, unsigned long secnum)
{
    unsigned int flags;
    void *buf = dc_cache_probe(IF_MV(fat_bpb->volume,) secnum, &flags);

    if (!flags)
    {
        int rc = storage_read_sectors(IF_MD(fat_bpb->drive,)
                                      secnum + fat_bpb->startsector, 1, buf);
        if (UNLIKELY(rc < 0))
        {
            DEBUGF("%s() - Could not read sector %ld"
                   " (error %d)\n", __func__, secnum, rc);
            dc_discard_buf(buf);
            return NULL;
        }
    }

    return buf;
}

/* returns a raw buffer for a sector; buffer counts as INUSE but filesystem
 * contents are NOT loaded before returning - use when completely overwriting
 * a sector's contents in order to avoid a fill */
static void * cache_sector_buffer(IF_MV(struct bpb *fat_bpb,)
                                  unsigned long secnum)
{
    unsigned int flags;
    return dc_cache_probe(IF_MV(fat_bpb->volume,) secnum, &flags);
}

/* flush a cache buffer to storage */
void dc_writeback_callback(IF_MV(int volume,) unsigned long sector, void *buf)
{
    struct bpb * const fat_bpb = &fat_bpbs[IF_MV_VOL(volume)];
    unsigned int copies = !IS_FAT_SECTOR(fat_bpb, sector) ?
                                1 : fat_bpb->bpb_numfats;

    sector += fat_bpb->startsector;

    while (1)
    {
        int rc = storage_write_sectors(IF_MD(fat_bpb->drive,) sector, 1, buf);
        if (rc < 0)
        {
            panicf("%s() - Could not write sector %ld"
                   " (error %d)\n", __func__, sector, rc);
        }

        if (--copies == 0)
            break;

        /* Update next FAT */
        sector += fat_bpb->fatsize;
    }
}

static void raw_dirent_set_fstclus(union raw_dirent *ent, long fstclus)
{
    ent->fstclushi = htole16(fstclus >> 16);
    ent->fstcluslo = htole16(fstclus & 0xffff);
}

static int bpb_is_sane(struct bpb *fat_bpb)
{
    if (fat_bpb->bpb_bytspersec % SECTOR_SIZE)
    {
        DEBUGF("%s() - Error: sector size is not sane (%lu)\n",
               __func__, fat_bpb->bpb_bytspersec);
        return -1;
    }

    if (fat_bpb->bpb_secperclus * fat_bpb->bpb_bytspersec > 128*1024ul)
    {
        DEBUGF("%s() - Error: cluster size is larger than 128K "
               "(%lu * %lu = %lu)\n", __func__,
               fat_bpb->bpb_bytspersec, fat_bpb->bpb_secperclus,
               fat_bpb->bpb_bytspersec * fat_bpb->bpb_secperclus);
        return -2;
    }

    if (fat_bpb->bpb_numfats != 2)
    {
        DEBUGF("%s() - Warning: NumFATS is not 2 (%u)\n",
               __func__, fat_bpb->bpb_numfats);
    }

    if (fat_bpb->bpb_media != 0xf0 && fat_bpb->bpb_media < 0xf8)
    {
        DEBUGF("%s() - Warning: Non-standard media type "
               "(0x%02x)\n", __func__, fat_bpb->bpb_media);
    }

    if (fat_bpb->last_word != 0xaa55)
    {
        DEBUGF("%s() - Error: Last word is not "
               "0xaa55 (0x%04x)\n", __func__, fat_bpb->last_word);
        return -3;
    }

    if (fat_bpb->fsinfo.freecount >
        (fat_bpb->totalsectors - fat_bpb->firstdatasector) /
        fat_bpb->bpb_secperclus)
    {
        DEBUGF("%s() - Error: FSInfo.Freecount > disk size "
               "(0x%04lx)\n", __func__,
               (unsigned long)fat_bpb->fsinfo.freecount);
        return -4;
    }

    return 0;
}

static uint8_t shortname_checksum(const unsigned char *shortname)
{
    /* calculate shortname checksum */
    uint8_t chksum = 0;

    for (unsigned int i = 0; i < 11; i++)
        chksum = (chksum << 7) + (chksum >> 1) + shortname[i];

    return chksum;
}

static void parse_short_direntry(const union raw_dirent *ent,
                                 struct fat_direntry *fatent)
{
    fatent->attr         = ent->attr;
    fatent->crttimetenth = ent->crttimetenth;
    fatent->crttime      = letoh16(ent->crttime);
    fatent->crtdate      = letoh16(ent->crtdate);
    fatent->lstaccdate   = letoh16(ent->lstaccdate);
    fatent->wrttime      = letoh16(ent->wrttime);
    fatent->wrtdate      = letoh16(ent->wrtdate);
    fatent->filesize     = letoh32(ent->filesize);
    fatent->firstcluster = ((uint32_t)letoh16(ent->fstcluslo)      ) |
                           ((uint32_t)letoh16(ent->fstclushi) << 16);

    /* fix the name */
    bool lowercase = ent->ntres & FAT_NTRES_LC_NAME;
    unsigned char c = ent->name[0];

    if (c == 0x05)  /* special kanji char */
        c = 0xe5;

    int j = 0;

    for (int i = 0; c != ' '; c = ent->name[i])
    {
        fatent->shortname[j++] = lowercase ? tolower(c) : c;

        if (++i >= 8)
            break;
    }

    if (ent->name[8] != ' ')
    {
        lowercase = ent->ntres & FAT_NTRES_LC_EXT;
        fatent->shortname[j++] = '.';

        for (int i = 8; i < 11 && (c = ent->name[i]) != ' '; i++)
            fatent->shortname[j++] = lowercase ? tolower(c) : c;
    }

    fatent->shortname[j] = 0;
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
static void NO_INLINE fatlong_parse_start(struct fatlong_parse_state *lnparse)
{
    /* no inline so gcc can't figure out what isn't initialized here;
       ord_max is king as to the validity of all other fields */
    lnparse->ord_max = -1; /* one resync per parse operation */
}

/* convert the FAT long name entry to a contiguous segment */
static bool fatlong_parse_entry(struct fatlong_parse_state *lnparse,
                                const union raw_dirent *ent,
                                struct fat_direntry *fatent)
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

    uint16_t *ucsp = fatent->ucssegs[ord - 1 + 5];
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
static bool fatlong_parse_finish(struct fatlong_parse_state *lnparse,
                                 const union raw_dirent *ent,
                                 struct fat_direntry *fatent)
{
    parse_short_direntry(ent, fatent);

    /* ord_max must not have been set to <= 0 because of any earlier problems
       and the ordinal immediately before the shortname entry must be 1 */
    if (lnparse->ord_max <= 0 || lnparse->ord != 1)
        return false;

     /* check the longname checksums against the shortname checksum */
    if (lnparse->chksum != shortname_checksum(ent->name))
        return false;

    /* longname is good so far so convert all the segments to UTF-8 */
    unsigned char * const name = fatent->name;
    unsigned char *p = name;

    /* ensure the last segment is NULL-terminated if it is filled */
    fatent->ucssegs[lnparse->ord_max + 5][0] = 0x0000;

    for (uint16_t *ucsp = fatent->ucssegs[5], ucc = *ucsp;
         ucc; ucc = *++ucsp)
    {
        /* end should be hit before ever seeing padding */
        if (ucc == 0xffff)
            return false;

        if ((p = utf8encode(ucc, p)) - name > FAT_DIRENTRY_NAME_MAX)
            return false;
    }

    /* longname ok */
    *p = '\0';
    return true;
}

static unsigned long cluster2sec(struct bpb *fat_bpb, long cluster)
{
    long zerocluster = 2;

    /* negative clusters (FAT16 root dir) don't get the 2 offset */
#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb->is_fat16 && cluster < 0)
    {
        zerocluster = 0;
    }
    else
#endif /* HAVE_FAT16SUPPORT */
    if ((unsigned long)cluster > fat_bpb->dataclusters + 1)
    {
        DEBUGF( "%s() - Bad cluster number (%ld)\n", __func__, cluster);
        return 0;
    }

    return (unsigned long)(cluster - zerocluster)*fat_bpb->bpb_secperclus
            + fat_bpb->firstdatasector;
}

#ifdef HAVE_FAT16SUPPORT
static long get_next_cluster16(struct bpb *fat_bpb, long startcluster)
{
    /* if FAT16 root dir, dont use the FAT */
    if (startcluster < 0)
        return startcluster + 1;

    unsigned long entry = startcluster;
    unsigned long sector = entry / CLUSTERS_PER_FAT16_SECTOR;
    unsigned long offset = entry % CLUSTERS_PER_FAT16_SECTOR;

    dc_lock_cache();

    uint16_t *sec = cache_sector(fat_bpb, sector + fat_bpb->fatrgnstart);
    if (!sec)
    {
        dc_unlock_cache();
        DEBUGF("%s: Could not cache sector %lu\n", __func__, sector);
        return -1;
    }

    long next = letoh16(sec[offset]);

    /* is this last cluster in chain? */
    if (next >= FAT16_EOF_MARK)
        next = 0;

    dc_unlock_cache();
    return next;
}

static long find_free_cluster16(struct bpb *fat_bpb, long startcluster)
{
    unsigned long entry = startcluster;
    unsigned long sector = entry / CLUSTERS_PER_FAT16_SECTOR;
    unsigned long offset = entry % CLUSTERS_PER_FAT16_SECTOR;

    for (unsigned long i = 0; i < fat_bpb->fatsize; i++)
    {
        unsigned long nr = (i + sector) % fat_bpb->fatsize;
        uint16_t *sec = cache_sector(fat_bpb, nr + fat_bpb->fatrgnstart);
        if (!sec)
            break;

        for (unsigned long j = 0; j < CLUSTERS_PER_FAT16_SECTOR; j++)
        {
            unsigned long k = (j + offset) % CLUSTERS_PER_FAT16_SECTOR;

            if (letoh16(sec[k]) == 0x0000)
            {
                unsigned long c = nr * CLUSTERS_PER_FAT16_SECTOR + k;
                 /* Ignore the reserved clusters 0 & 1, and also
                    cluster numbers out of bounds */
                if (c < 2 || c > fat_bpb->dataclusters + 1)
                    continue;

                DEBUGF("%s(%lx) == %lx\n", __func__, startcluster, c);

                fat_bpb->fsinfo.nextfree = c;
                return c;
            }
        }

        offset = 0;
    }

    DEBUGF("%s(%lx) == 0\n", __func__, startcluster);
    return 0; /* 0 is an illegal cluster number */
}

static int update_fat_entry16(struct bpb *fat_bpb, unsigned long entry,
                              unsigned long val)
{
    unsigned long sector = entry / CLUSTERS_PER_FAT16_SECTOR;
    unsigned long offset = entry % CLUSTERS_PER_FAT16_SECTOR;

    val &= 0xFFFF;

    DEBUGF("%s(entry:%lx,val:%lx)\n", __func__, entry, val);

    if (entry == val)
        panicf("Creating FAT16 loop: %lx,%lx\n", entry, val);

    if (entry < 2)
        panicf("Updating reserved FAT16 entry %lu\n", entry);

    dc_lock_cache();

    int16_t *sec = cache_sector(fat_bpb, sector + fat_bpb->fatrgnstart);
    if (!sec)
    {
        dc_unlock_cache();
        DEBUGF("Could not cache sector %lu\n", sector);
        return -1;
    }

    uint16_t curval = letoh16(sec[offset]);

    if (val)
    {
        /* being allocated */
        if (curval == 0x0000 && fat_bpb->fsinfo.freecount > 0)
            fat_bpb->fsinfo.freecount--;
    }
    else
    {
        /* being freed */
        if (curval != 0x0000)
            fat_bpb->fsinfo.freecount++;
    }

    DEBUGF("%lu free clusters\n", (unsigned long)fat_bpb->fsinfo.freecount);

    sec[offset] = htole16(val);
    dc_dirty_buf(sec);

    dc_unlock_cache();
    return 0;
}

static void fat_recalc_free_internal16(struct bpb *fat_bpb)
{
    unsigned long free = 0;

    for (unsigned long i = 0; i < fat_bpb->fatsize; i++)
    {
        uint16_t *sec = cache_sector(fat_bpb, i + fat_bpb->fatrgnstart);
        if (!sec)
            break;

        for (unsigned long j = 0; j < CLUSTERS_PER_FAT16_SECTOR; j++)
        {
            unsigned long c = i * CLUSTERS_PER_FAT16_SECTOR + j;

            if (c < 2 || c > fat_bpb->dataclusters + 1) /* nr 0 is unused */
                continue;

            if (letoh16(sec[j]) != 0x0000)
                continue;

            free++;
            if (fat_bpb->fsinfo.nextfree == 0xffffffff)
                fat_bpb->fsinfo.nextfree = c;
        }
    }

    fat_bpb->fsinfo.freecount = free;
}
#endif /* HAVE_FAT16SUPPORT */

static void update_fsinfo32(struct bpb *fat_bpb)
{
    uint8_t *fsinfo = cache_sector(fat_bpb, fat_bpb->bpb_fsinfo);
    if (!fsinfo)
    {
        DEBUGF("%s() - Couldn't read FSInfo", __func__);
        return;
    }

    INT322BYTES(fsinfo, FSINFO_FREECOUNT, fat_bpb->fsinfo.freecount);
    INT322BYTES(fsinfo, FSINFO_NEXTFREE, fat_bpb->fsinfo.nextfree);
    dc_dirty_buf(fsinfo);
}

static long get_next_cluster32(struct bpb *fat_bpb, long startcluster)
{
    unsigned long entry = startcluster;
    unsigned long sector = entry / CLUSTERS_PER_FAT_SECTOR;
    unsigned long offset = entry % CLUSTERS_PER_FAT_SECTOR;

    dc_lock_cache();

    uint32_t *sec = cache_sector(fat_bpb, sector + fat_bpb->fatrgnstart);
    if (!sec)
    {
        dc_unlock_cache();
        DEBUGF("%s: Could not cache sector %lu\n", __func__, sector);
        return -1;
    }

    long next = letoh32(sec[offset]) & 0x0fffffff;

    /* is this last cluster in chain? */
    if (next >= FAT_EOF_MARK)
        next = 0;

    dc_unlock_cache();
    return next;
}

static long find_free_cluster32(struct bpb *fat_bpb, long startcluster)
{
    unsigned long entry = startcluster;
    unsigned long sector = entry / CLUSTERS_PER_FAT_SECTOR;
    unsigned long offset = entry % CLUSTERS_PER_FAT_SECTOR;

    for (unsigned long i = 0; i < fat_bpb->fatsize; i++)
    {
        unsigned long nr = (i + sector) % fat_bpb->fatsize;
        uint32_t *sec = cache_sector(fat_bpb, nr + fat_bpb->fatrgnstart);
        if (!sec)
            break;

        for (unsigned long j = 0; j < CLUSTERS_PER_FAT_SECTOR; j++)
        {
            unsigned long k = (j + offset) % CLUSTERS_PER_FAT_SECTOR;

            if (!(letoh32(sec[k]) & 0x0fffffff))
            {
                unsigned long c = nr * CLUSTERS_PER_FAT_SECTOR + k;
                 /* Ignore the reserved clusters 0 & 1, and also
                    cluster numbers out of bounds */
                if (c < 2 || c > fat_bpb->dataclusters + 1)
                    continue;

                DEBUGF("%s(%lx) == %lx\n", __func__, startcluster, c);

                fat_bpb->fsinfo.nextfree = c;
                return c;
            }
        }

        offset = 0;
    }

    DEBUGF("%s(%lx) == 0\n", __func__, startcluster);
    return 0; /* 0 is an illegal cluster number */
}

static int update_fat_entry32(struct bpb *fat_bpb, unsigned long entry,
                              unsigned long val)
{
    unsigned long sector = entry / CLUSTERS_PER_FAT_SECTOR;
    unsigned long offset = entry % CLUSTERS_PER_FAT_SECTOR;

    DEBUGF("%s(entry:%lx,val:%lx)\n", __func__, entry, val);

    if (entry == val)
        panicf("Creating FAT32 loop: %lx,%lx\n", entry, val);

    if (entry < 2)
        panicf("Updating reserved FAT32 entry %lu\n", entry);

    dc_lock_cache();

    uint32_t *sec = cache_sector(fat_bpb, sector + fat_bpb->fatrgnstart);
    if (!sec)
    {
        dc_unlock_cache();
        DEBUGF("Could not cache sector %lu\n", sector);
        return -1;
    }

    uint32_t curval = letoh32(sec[offset]);

    if (val)
    {
        /* being allocated */
        if (!(curval & 0x0fffffff) && fat_bpb->fsinfo.freecount > 0)
            fat_bpb->fsinfo.freecount--;
    }
    else
    {
        /* being freed */
        if (curval & 0x0fffffff)
            fat_bpb->fsinfo.freecount++;
    }

    DEBUGF("%lu free clusters\n", (unsigned long)fat_bpb->fsinfo.freecount);

    /* don't change top 4 bits */
    sec[offset] = htole32((curval & 0xf0000000) | (val & 0x0fffffff));
    dc_dirty_buf(sec);

    dc_unlock_cache();
    return 0;
}

static void fat_recalc_free_internal32(struct bpb *fat_bpb)
{
    unsigned long free = 0;

    for (unsigned long i = 0; i < fat_bpb->fatsize; i++)
    {
        uint32_t *sec = cache_sector(fat_bpb, i + fat_bpb->fatrgnstart);
        if (!sec)
            break;

        for (unsigned long j = 0; j < CLUSTERS_PER_FAT_SECTOR; j++)
        {
            unsigned long c = i * CLUSTERS_PER_FAT_SECTOR + j;

            if (c < 2 || c > fat_bpb->dataclusters + 1) /* nr 0 is unused */
                continue;

            if (letoh32(sec[j]) & 0x0fffffff)
                continue;

            free++;
            if (fat_bpb->fsinfo.nextfree == 0xffffffff)
                fat_bpb->fsinfo.nextfree = c;
        }
    }

    fat_bpb->fsinfo.freecount = free;
    update_fsinfo32(fat_bpb);
}

static int fat_mount_internal(struct bpb *fat_bpb)
{
    int rc;
    /* safe to grab buffer: bpb is irrelevant and no sector will be cached
       for this volume since it isn't mounted */
    uint8_t * const buf = dc_get_buffer();
    if (!buf)
        FAT_ERROR(-1);

    /* Read the sector */
    rc = storage_read_sectors(IF_MD(fat_bpb->drive,) fat_bpb->startsector,
                              1, buf);
    if(rc)
    {
        DEBUGF("%s() - Couldn't read BPB"
               " (err %d)\n", __func__, rc);
        FAT_ERROR(rc * 10 - 2);
    }

    fat_bpb->bpb_bytspersec = BYTES2INT16(buf, BPB_BYTSPERSEC);
    unsigned long secmult = fat_bpb->bpb_bytspersec / SECTOR_SIZE;
    /* Sanity check is performed later */

    fat_bpb->bpb_secperclus = secmult * buf[BPB_SECPERCLUS];
    fat_bpb->bpb_rsvdseccnt = secmult * BYTES2INT16(buf, BPB_RSVDSECCNT);
    fat_bpb->bpb_numfats    = buf[BPB_NUMFATS];
    fat_bpb->bpb_media      = buf[BPB_MEDIA];
    fat_bpb->bpb_fatsz16    = secmult * BYTES2INT16(buf, BPB_FATSZ16);
    fat_bpb->bpb_fatsz32    = secmult * BYTES2INT32(buf, BPB_FATSZ32);
    fat_bpb->bpb_totsec16   = secmult * BYTES2INT16(buf, BPB_TOTSEC16);
    fat_bpb->bpb_totsec32   = secmult * BYTES2INT32(buf, BPB_TOTSEC32);
    fat_bpb->last_word      = BYTES2INT16(buf, BPB_LAST_WORD);

    /* calculate a few commonly used values */
    if (fat_bpb->bpb_fatsz16 != 0)
        fat_bpb->fatsize = fat_bpb->bpb_fatsz16;
    else
        fat_bpb->fatsize = fat_bpb->bpb_fatsz32;

    fat_bpb->fatrgnstart = fat_bpb->bpb_rsvdseccnt;
    fat_bpb->fatrgnend   = fat_bpb->bpb_rsvdseccnt + fat_bpb->fatsize;

    if (fat_bpb->bpb_totsec16 != 0)
        fat_bpb->totalsectors = fat_bpb->bpb_totsec16;
    else
        fat_bpb->totalsectors = fat_bpb->bpb_totsec32;

    unsigned int rootdirsectors = 0;
#ifdef HAVE_FAT16SUPPORT
    fat_bpb->bpb_rootentcnt = BYTES2INT16(buf, BPB_ROOTENTCNT);

    if (!fat_bpb->bpb_bytspersec)
        FAT_ERROR(-3);

    rootdirsectors = secmult * ((fat_bpb->bpb_rootentcnt * DIR_ENTRY_SIZE
                     + fat_bpb->bpb_bytspersec - 1) / fat_bpb->bpb_bytspersec);
#endif /* HAVE_FAT16SUPPORT */

    fat_bpb->firstdatasector = fat_bpb->bpb_rsvdseccnt
        + fat_bpb->bpb_numfats * fat_bpb->fatsize
        + rootdirsectors;

    /* Determine FAT type */
    unsigned long datasec = fat_bpb->totalsectors - fat_bpb->firstdatasector;

    if (!fat_bpb->bpb_secperclus)
        FAT_ERROR(-4);

    fat_bpb->dataclusters = datasec / fat_bpb->bpb_secperclus;

#ifdef TEST_FAT
    /*
      we are sometimes testing with "illegally small" fat32 images,
      so we don't use the proper fat32 test case for test code
    */
    if (fat_bpb->bpb_fatsz16)
#else /* !TEST_FAT */
    if (fat_bpb->dataclusters < 65525)
#endif /* TEST_FAT */
    { /* FAT16 */
#ifdef HAVE_FAT16SUPPORT
        fat_bpb->is_fat16 = true;
        if (fat_bpb->dataclusters < 4085)
        { /* FAT12 */
            DEBUGF("This is FAT12. Go away!\n");
            FAT_ERROR(-5);
        }
#else /* !HAVE_FAT16SUPPORT */
        DEBUGF("This is not FAT32. Go away!\n");
        FAT_ERROR(-6);
#endif /* HAVE_FAT16SUPPORT */
    }

#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb->is_fat16)
    {
        /* FAT16 specific part of BPB */
        fat_bpb->rootdirsector = fat_bpb->bpb_rsvdseccnt
            + fat_bpb->bpb_numfats * fat_bpb->bpb_fatsz16;
        long dirclusters = ((rootdirsectors + fat_bpb->bpb_secperclus - 1)
            / fat_bpb->bpb_secperclus); /* rounded up, to full clusters */
        /* I assign negative pseudo cluster numbers for the root directory,
           their range is counted upward until -1. */
        fat_bpb->bpb_rootclus = 0 - dirclusters; /* backwards, before the data */
        fat_bpb->rootdirsectornum = dirclusters * fat_bpb->bpb_secperclus
            - rootdirsectors;
    }
    else
#endif /* HAVE_FAT16SUPPORT */
    {
        /* FAT32 specific part of BPB */
        fat_bpb->bpb_rootclus  = BYTES2INT32(buf, BPB_ROOTCLUS);
        fat_bpb->bpb_fsinfo    = secmult * BYTES2INT16(buf, BPB_FSINFO);
        fat_bpb->rootdirsector = cluster2sec(fat_bpb, fat_bpb->bpb_rootclus);
    }

    rc = bpb_is_sane(fat_bpb);
    if (rc < 0)
    {
        DEBUGF("%s: BPB is insane!\n", __func__);
        FAT_ERROR(rc * 10 - 7);
    }

#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb->is_fat16)
    {
        fat_bpb->fsinfo.freecount = 0xffffffff; /* force recalc later */
        fat_bpb->fsinfo.nextfree = 0xffffffff;
    }
    else
#endif /* HAVE_FAT16SUPPORT */
    {
        /* Read the fsinfo sector */
        rc = storage_read_sectors(IF_MD(fat_bpb->drive,)
                fat_bpb->startsector + fat_bpb->bpb_fsinfo, 1, buf);

        if (rc < 0)
        {
            DEBUGF("%s() - Couldn't read FSInfo"
                   " (error code %d)\n", __func__, rc);
            FAT_ERROR(rc * 10 - 8);
        }

        fat_bpb->fsinfo.freecount = BYTES2INT32(buf, FSINFO_FREECOUNT);
        fat_bpb->fsinfo.nextfree = BYTES2INT32(buf, FSINFO_NEXTFREE);
    }

#ifdef HAVE_FAT16SUPPORT
    /* Fix up calls that change per FAT type */
    if (fat_bpb->is_fat16)
    {
        BPB_FN_SET16(fat_bpb, get_next_cluster);
        BPB_FN_SET16(fat_bpb, find_free_cluster);
        BPB_FN_SET16(fat_bpb, update_fat_entry);
        BPB_FN_SET16(fat_bpb, fat_recalc_free_internal);
    }
    else
    {
        BPB_FN_SET32(fat_bpb, get_next_cluster);
        BPB_FN_SET32(fat_bpb, find_free_cluster);
        BPB_FN_SET32(fat_bpb, update_fat_entry);
        BPB_FN_SET32(fat_bpb, fat_recalc_free_internal);
    }
#endif /* HAVE_FAT16SUPPORT */

    rc = 0;
fat_error:
    dc_release_buffer(buf);
    return rc;
}

static union raw_dirent * cache_direntry(struct bpb *fat_bpb,
                                         struct fat_filestr *filestr,
                                         unsigned int entry)
{
    filestr->eof = false;

    if (entry >= MAX_DIRENTRIES)
    {
        DEBUGF("%s() - Dir is too large (entry %u)\n", __func__, entry);
        return NULL;
    }

    unsigned long sector = entry / DIR_ENTRIES_PER_SECTOR;

    if (fat_query_sectornum(filestr) != sector + 1)
    {
        int rc = fat_seek(filestr, sector + 1);
        if (rc < 0)
        {
            if (rc == FAT_SEEK_EOF)
            {
                DEBUGF("%s() - End of dir (entry %u)\n", __func__, entry);
                fat_seek(filestr, sector);
                filestr->eof = true;
            }

            return NULL;
        }
    }

    union raw_dirent *ent = cache_sector(fat_bpb, filestr->lastsector);

    if (ent)
        ent += entry % DIR_ENTRIES_PER_SECTOR;

    return ent;
}

static long next_write_cluster(struct bpb *fat_bpb, long oldcluster)
{
    DEBUGF("%s(old:%lx)\n", __func__, oldcluster);

    long cluster = 0;

    /* cluster already allocated? */
    if (oldcluster)
        cluster = get_next_cluster(fat_bpb, oldcluster);

    if (!cluster)
    {
        /* passed end of existing entries and now need to append */
    #ifdef HAVE_FAT16SUPPORT
        if (UNLIKELY(oldcluster < 0))
            return 0; /* negative, pseudo-cluster of the root dir */
                      /* impossible to append something to the root */
    #endif /* HAVE_FAT16SUPPORT */

        dc_lock_cache();

        long findstart = oldcluster > 0 ?
            oldcluster + 1 : (long)fat_bpb->fsinfo.nextfree;

        cluster = find_free_cluster(fat_bpb, findstart);

        if (cluster)
        {
            /* create the cluster chain */
            if (oldcluster)
                update_fat_entry(fat_bpb, oldcluster, cluster);

            update_fat_entry(fat_bpb, cluster, FAT_EOF_MARK);
        }
        else
        {
        #ifdef TEST_FAT
            if (fat_bpb->fsinfo.freecount > 0)
                panicf("There is free space, but find_free_cluster() "
                       "didn't find it!\n");
        #endif
            DEBUGF("Disk full!\n");
        }

        dc_unlock_cache();
    }

    return cluster;
}

/* extend dir file by one cluster and clear it; file position should be at the
   current last cluster before calling and size of dir checked */
static int fat_extend_dir(struct bpb *fat_bpb, struct fat_filestr *dirstr)
{
    DEBUGF("%s()\n", __func__);

    int rc;

    long cluster    = dirstr->lastcluster;
    long newcluster = next_write_cluster(fat_bpb, cluster);

    if (!newcluster)
    {
        /* no more room or something went wrong */
        DEBUGF("Out of space\n");
        FAT_ERROR(FAT_RC_ENOSPC);
    }

    /* we must clear whole clusters */
    unsigned long startsector = cluster2sec(fat_bpb, newcluster);
    unsigned long sector      = startsector - 1;

    if (startsector == 0)
        FAT_ERROR(-1);

    for (unsigned int i = 0; i < fat_bpb->bpb_secperclus; i++)
    {
        dc_lock_cache();

        void *sec = cache_sector_buffer(IF_MV(fat_bpb,) ++sector);
        if (!sec)
        {
            dc_unlock_cache();
            DEBUGF("Cannot clear cluster %ld\n", newcluster);
            update_fat_entry(fat_bpb, newcluster, 0);
            FAT_ERROR(-2);
        }

        memset(sec, 0, SECTOR_SIZE);
        dc_dirty_buf(sec);
        dc_unlock_cache();
    }

    if (!dirstr->fatfilep->firstcluster)
        dirstr->fatfilep->firstcluster = newcluster;

    dirstr->lastcluster = newcluster;
    dirstr->lastsector  = sector;
    dirstr->clusternum++;
    dirstr->sectornum   = sector - startsector;
    dirstr->eof         = false;

    rc = 0;
fat_error:
    return rc;
}

static void fat_open_internal(IF_MV(int volume,) long startcluster,
                              struct fat_file *file)
{
#ifdef HAVE_MULTIVOLUME
    file->volume       = volume;
#endif
    file->firstcluster = startcluster;
    file->dircluster   = 0;
    file->e.entry      = 0;
    file->e.entries    = 0;
}

#if CONFIG_RTC
static void fat_time(uint16_t *date, uint16_t *time, int16_t *tenth)
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

static void fat_time(uint16_t *date, uint16_t *time, int16_t *tenth)
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

static int write_longname(struct bpb *fat_bpb, struct fat_filestr *parentstr,
                          struct fat_file *file, const unsigned char *name,
                          unsigned long ucslen, const unsigned char *shortname,
                          union raw_dirent *srcent, uint8_t attr,
                          unsigned int flags)
{
    DEBUGF("%s(file:%lx, first:%d, num:%d, name:%s)\n", __func__,
           file->firstcluster, file->e.entry - file->e.entries + 1,
           file->e.entries, name);

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

    dc_lock_cache();

    const unsigned int longentries = file->e.entries - 1;
    const unsigned int firstentry  = file->e.entry - longentries;

    /* longame entries */
    for (unsigned int i = 0; i < longentries; i++)
    {
        ent = cache_direntry(fat_bpb, parentstr, firstentry + i);
        if (!ent)
            FAT_ERROR(-2);

        /* verify this entry is free */
        if (ent->name[0] && ent->name[0] != 0xe5)
        {
            panicf("Dir entry %d in sector %x is not free! "
                   "%02x %02x %02x %02x",
                   i + firstentry, (unsigned)parentstr->lastsector,
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

        dc_dirty_buf(ent);
        DEBUGF("Longname entry %d\n", ord);
    }

    /* shortname entry */
    DEBUGF("Shortname '%s'\n", shortname);

    ent = cache_direntry(fat_bpb, parentstr, file->e.entry);
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
    raw_dirent_set_fstclus(ent, file->firstcluster);

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

    dc_dirty_buf(ent);

    rc = 0;
fat_error:
    dc_unlock_cache();
    return rc;
}

static int add_dir_entry(struct bpb *fat_bpb, struct fat_filestr *parentstr,
                         struct fat_file *file, const char *name,
                         union raw_dirent *srcent, uint8_t attr,
                         unsigned int flags)
{
    DEBUGF("%s(name:\"%s\",first:%lx)\n", __func__, name,
           file->firstcluster);

    int rc;

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
        rc = check_longname(name);
        if (rc < 0)
            FAT_ERROR(rc * 10 - 1); /* filename is invalid */

        create_dos_name(basisname, name, &n);
        randomize_dos_name(shortname, basisname, &n);

        /* one dir entry needed for every 13 characters of filename,
           plus one entry for the short name */
        ucslen = utf8length(name);
        if (ucslen > 255)
            FAT_ERROR(-2); /* name is too long */

        entries_needed = (ucslen + FATLONG_NAME_CHARS - 1)
                            / FATLONG_NAME_CHARS + 1;
    }

    int entry = 0, entries_found = 0, firstentry = -1;
    const int entperclus = DIR_ENTRIES_PER_SECTOR*fat_bpb->bpb_secperclus;

    /* step 1: search for a sufficiently-long run of free entries and check
               for duplicate shortname */
    dc_lock_cache();

    for (bool done = false; !done;)
    {
        union raw_dirent *ent = cache_direntry(fat_bpb, parentstr, entry);

        if (!ent)
        {
            if (parentstr->eof)
            {
                DEBUGF("End of dir (entry %d)\n", entry);
                break;
            }

            DEBUGF("Couldn't read dir (entry %d)\n", entry);
            dc_unlock_cache();
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

        if (firstentry < 0 && entries_found >= entries_needed)
        {
            /* found adequate space; point to initial free entry */
            firstentry = entry - entries_found;
        }
    }

    dc_unlock_cache();

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
            rc = fat_extend_dir(fat_bpb, parentstr);
            if (rc == FAT_RC_ENOSPC)
                FAT_ERROR(RC);
            else if (rc < 0)
                FAT_ERROR(rc * 10 - 5);

            entries_found += entperclus;
            entry += entperclus;
        }

        firstentry = entry - entries_found;
    }

    /* remember the parent directory entry information */
#ifdef HAVE_MULTIVOLUME
    file->volume     = parentstr->fatfilep->volume;
#endif
    file->dircluster = parentstr->fatfilep->firstcluster;
    file->e.entry    = firstentry + entries_needed - 1;
    file->e.entries  = entries_needed;

    /* step 3: add entry */
    DEBUGF("Adding longname to entry %d\n", firstentry);
    rc = write_longname(fat_bpb, parentstr, file, name, ucslen,
                        shortname, srcent, attr, flags);
    if (rc < 0)
        FAT_ERROR(rc * 10 - 6);

    DEBUGF("Added new dir entry %u; using %u entries\n",
           file->e.entry, file->e.entries);

    rc = 0;
fat_error:
    return rc;
}

static int update_short_entry(struct bpb *fat_bpb, struct fat_file *file,
                              uint32_t size, struct fat_direntry *fatent)
{
    DEBUGF("%s(cluster:%lx entry:%d size:%ld)\n",
           __func__, file->firstcluster, file->e.entry, size);

    int rc;

#if CONFIG_RTC
    uint16_t time = 0;
    uint16_t date = 0;
#else
    /* get old time to increment from */
    uint16_t time = letoh16(fatent->wrttime);
    uint16_t date = letoh16(fatent->wrtdate);
#endif
    fat_time(&date, &time, NULL);
    date = htole16(date);
    time = htole16(time);

    /* open the parent directory */
    struct fat_file parent;
    fat_open_internal(IF_MV(file->volume,) file->dircluster, &parent);

    struct fat_filestr parentstr;
    fat_filestr_init(&parentstr, &parent);

    dc_lock_cache();

    union raw_dirent *ent = cache_direntry(fat_bpb, &parentstr, file->e.entry);
    if (!ent)
        FAT_ERROR(-1);

    if (!ent->name[0] || ent->name[0] == 0xe5)
        panicf("Updating size on empty dir entry %d\n", file->e.entry);

    /* basic file data */
    raw_dirent_set_fstclus(ent, file->firstcluster);
    ent->filesize = htole32(size);

    /* time and date info */
    ent->wrttime    = time;
    ent->wrtdate    = date;
    ent->lstaccdate = date;

    if (fatent)
    {
        fatent->name[0] = '\0'; /* not gonna bother here */
        parse_short_direntry(ent, fatent);
    }

    dc_dirty_buf(ent);

    rc = 0;
fat_error:
    dc_unlock_cache();
    return rc;
}

static int free_direntries(struct bpb *fat_bpb, struct fat_file *file)
{
    /* open the parent directory */
    struct fat_file parent;
    fat_open_internal(IF_MV(file->volume,) file->dircluster, &parent);

    struct fat_filestr parentstr;
    fat_filestr_init(&parentstr, &parent);

    for (unsigned int entries = file->e.entries,
                      entry = file->e.entry - entries + 1;
         entries; entries--, entry++)
    {
        DEBUGF("Clearing dir entry %d (%d/%d)\n",
               entry, entries - file->e.entries + 1, file->e.entries);


        dc_lock_cache();

        union raw_dirent *ent = cache_direntry(fat_bpb, &parentstr, entry);
        if (!ent)
        {
            dc_unlock_cache();

            if (entries == file->e.entries)
                return -1; /* nothing at all freed */

            /* longname already destroyed; revert to shortname */
            file->e.entries = 1;
            return 0;
        }

        ent->data[0] = 0xe5;

        dc_dirty_buf(ent);
        dc_unlock_cache();
    }

    /* directory entry info is now gone */
    file->dircluster = 0;
    file->e.entry    = FAT_DIRSCAN_RW_VAL;
    file->e.entries  = 0;

    return 1;
}

static int free_cluster_chain(struct bpb *fat_bpb, long startcluster)
{
    for (long last = startcluster, next; last; last = next)
    {
        next = get_next_cluster(fat_bpb, last);
        int rc = update_fat_entry(fat_bpb, last, 0);
        if (LIKELY(rc >= 0 && !startcluster))
            continue;

        if (rc < 0)
            return startcluster ? -1 : 0;

        startcluster = 0;
    }

    return 1;
}


/** File entity functions **/

int fat_create_file(struct fat_file *parent, const char *name,
                    uint8_t attr, struct fat_file *file,
                    struct fat_direntry *fatent)
{
    DEBUGF("%s(\"%s\",%lx,%lx)\n", __func__, name, (long)file, (long)parent);
    struct bpb * const fat_bpb = FAT_BPB(parent->volume);
    if (!fat_bpb)
        return -1;

    int rc;

    fat_open_internal(IF_MV(parent->volume,) 0, file);

    struct fat_filestr parentstr;
    fat_filestr_init(&parentstr, parent);

    const bool isdir = attr & ATTR_DIRECTORY;
    unsigned int addflags = fatent ? DIRENT_RETURN : 0;
    union raw_dirent *newentp = (isdir || fatent) ?
                                alloca(sizeof (union raw_dirent)) : NULL;

    if (isdir)
    {
        struct fat_filestr dirstr;
        fat_filestr_init(&dirstr, file);

        /* create the first cluster */
        rc = fat_extend_dir(fat_bpb, &dirstr);
        if (rc == FAT_RC_ENOSPC)
            FAT_ERROR(RC);
        else if (rc < 0)
            FAT_ERROR(rc * 10 - 2);

        struct fat_file dummyfile;

        /* add the "." entry */
        fat_open_internal(IF_MV(0,) file->firstcluster, &dummyfile);

        /* this returns the short entry template for the remaining entries */
        rc = add_dir_entry(fat_bpb, &dirstr, &dummyfile, ".", newentp,
                           attr, DIRENT_RETURN);
        if (rc < 0)
            FAT_ERROR(rc * 10 - 3);

        /* and the ".." entry */
        /* the root cluster is cluster 0 in the ".." entry */
        fat_open_internal(IF_MV(0,)
            parent->firstcluster == fat_bpb->bpb_rootclus ?
                0 : parent->firstcluster, &dummyfile);

        rc = add_dir_entry(fat_bpb, &dirstr, &dummyfile, "..", newentp,
                           attr, DIRENT_TEMPL_TIMES);
        if (rc < 0)
            FAT_ERROR(rc * 10 - 4);

        addflags |= DIRENT_TEMPL_TIMES;
    }

    /* lastly, add the entry in the parent directory */
    rc = add_dir_entry(fat_bpb, &parentstr, file, name, newentp,
                       attr, addflags);
    if (rc == FAT_RC_ENOSPC)
        FAT_ERROR(RC);
    else if (rc < 0)
        FAT_ERROR(rc * 10 - 5);

    if (fatent)
    {
        strcpy(fatent->name, name);
        parse_short_direntry(newentp, fatent);
    }

    rc = 0;
fat_error:
    if (rc < 0)
        free_cluster_chain(fat_bpb, file->firstcluster);

    cache_commit(fat_bpb);
    return rc;
}

bool fat_dir_is_parent(const struct fat_file *dir, const struct fat_file *file)
{
    /* if the directory file's first cluster is the same as the file's
       directory cluster and they're on the same volume, 'dir' is its parent
       directory; the file must also have a dircluster (ie. not removed) */
    long dircluster = file->dircluster;
    return dircluster && dircluster == dir->firstcluster
            IF_MV( && dir->volume == file->volume );
}

bool fat_file_is_same(const struct fat_file *file1,
                      const struct fat_file *file2)
{
    /* first, triviality */
    if (file1 == file2)
        return true;

    /* if the directory info matches and the volumes are the same, file1 and
       file2 refer to the same file/directory */
    return file1->dircluster == file2->dircluster
           && file1->e.entry == file2->e.entry
           IF_MV( && file1->volume == file2->volume );
}

int fat_open(const struct fat_file *parent, long startcluster,
             struct fat_file *file)
{
    if (!parent)
        return -2; /* this does _not_ open any root */

    struct bpb * const fat_bpb = FAT_BPB(parent->volume);
    if (!fat_bpb)
        return -1;

    /* inherit basic parent information; dirscan info is expected to have been
       initialized beforehand (usually via scanning for the entry ;) */
#ifdef HAVE_MULTIVOLUME
    file->volume       = parent->volume;
#endif
    file->firstcluster = startcluster;
    file->dircluster   = parent->firstcluster;

    return 0;
}

int fat_open_rootdir(IF_MV(int volume,) struct fat_file *dir)
{
    struct bpb * const fat_bpb = FAT_BPB(volume);
    if (!fat_bpb)
        return -1;

    fat_open_internal(IF_MV(volume,) fat_bpb->bpb_rootclus, dir);
    return 0;
}

int fat_remove(struct fat_file *file, enum fat_remove_op what)
{
    struct bpb * const fat_bpb = FAT_BPB(file->volume);
    if (!fat_bpb)
        return -1;

    int rc;

    if (file->firstcluster == fat_bpb->bpb_rootclus)
    {
        DEBUGF("Trying to remove root of volume %d\n",
               IF_MV_VOL(file->volume));
        FAT_ERROR(-2);
    }

    if (file->dircluster && (what & FAT_RM_DIRENTRIES))
    {
        /* free everything in the parent directory */
        DEBUGF("Removing dir entries: %lX\n", file->dircluster);
        rc = free_direntries(fat_bpb, file);
        if (rc <= 0)
            FAT_ERROR(rc * 10 - 3);
    }

    if (file->firstcluster && (what & FAT_RM_DATA))
    {
        /* mark all clusters in the chain as free */
        DEBUGF("Removing cluster chain: %lX\n", file->firstcluster);
        rc = free_cluster_chain(fat_bpb, file->firstcluster);
        if (rc < 0)
            FAT_ERROR(rc * 10 - 4);

        /* at least the first cluster was freed */
        file->firstcluster = 0;

        if (rc == 0)
            FAT_ERROR(-5);
    }

    rc = 0;
fat_error:
    cache_commit(fat_bpb);
    return rc;
}

int fat_rename(struct fat_file *parent, struct fat_file *file,
               const unsigned char *newname)
{
    struct bpb * const fat_bpb = FAT_BPB(parent->volume);
    if (!fat_bpb)
        return -1;

    int rc;
    /* save old file; don't change it unless everything succeeds */
    struct fat_file newfile = *file;

#ifdef HAVE_MULTIVOLUME
    /* rename only works on the same volume */
    if (file->volume != parent->volume)
    {
        DEBUGF("No rename across volumes!\n");
        FAT_ERROR(-2);
    }
#endif

    /* root directories can't be renamed */
    if (file->firstcluster == fat_bpb->bpb_rootclus)
    {
        DEBUGF("Trying to rename root of volume %d\n",
               IF_MV_VOL(file->volume));
        FAT_ERROR(-3);
    }

    if (!file->dircluster)
    {
        /* file was removed but is still open */
        DEBUGF("File has no dir cluster!\n");
        FAT_ERROR(-4);
    }

    struct fat_file dir;
    struct fat_filestr dirstr;

    /* open old parent */
    fat_open_internal(IF_MV(file->volume,) file->dircluster, &dir);
    fat_filestr_init(&dirstr, &dir);

    /* fetch a copy of the existing short entry */
    dc_lock_cache();

    union raw_dirent *ent = cache_direntry(fat_bpb, &dirstr, file->e.entry);
    if (!ent)
    {
        dc_unlock_cache();
        FAT_ERROR(-5);
    }

    union raw_dirent rawent = *ent;

    dc_unlock_cache();

    /* create new name in new parent directory */
    fat_filestr_init(&dirstr, parent);
    rc = add_dir_entry(fat_bpb, &dirstr, &newfile, newname, &rawent,
                       0, DIRENT_TEMPL_CRT | DIRENT_TEMPL_WRT);
    if (rc == FAT_RC_ENOSPC)
        FAT_ERROR(RC);
    else if (rc < 0)
        FAT_ERROR(rc * 10 - 6);

    /* if renaming a directory and it was a move, update the '..' entry to
       keep it pointing to its parent directory */
    if ((rawent.attr & ATTR_DIRECTORY) && newfile.dircluster != file->dircluster)
    {
        /* open the dir that was renamed */
        fat_open_internal(IF_MV(newfile.volume,) newfile.firstcluster, &dir);
        fat_filestr_init(&dirstr, &dir);

        /* obtain dot-dot directory entry */
        dc_lock_cache();
        ent = cache_direntry(fat_bpb, &dirstr, 1);
        if (!ent)
        {
            dc_unlock_cache();
            FAT_ERROR(-7);
        }

        if (strncmp("..         ", ent->name, 11))
        {
            /* .. entry must be second entry according to FAT spec (p.29) */
            DEBUGF("Second dir entry is not double-dot!\n");
            dc_unlock_cache();
            FAT_ERROR(-8);
        }

        /* parent cluster is 0 if parent dir is the root - FAT spec (p.29) */
        long parentcluster = 0;
        if (parent->firstcluster != fat_bpb->bpb_rootclus)
            parentcluster = parent->firstcluster;

        raw_dirent_set_fstclus(ent, parentcluster);

        dc_dirty_buf(ent);
        dc_unlock_cache();
    }

    /* remove old name */
    rc = free_direntries(fat_bpb, file);
    if (rc <= 0)
        FAT_ERROR(rc * 10 - 9);

    /* finally, update old file with new directory entry info */
    *file = newfile;

    rc = 0;
fat_error:
    if (rc < 0 && !fat_file_is_same(&newfile, file))
        free_direntries(fat_bpb, &newfile);

    cache_commit(fat_bpb);
    return rc;
}


/** File stream functions **/

int fat_closewrite(struct fat_filestr *filestr, uint32_t size,
                   struct fat_direntry *fatentp)
{
    DEBUGF("%s(size=%ld)\n", __func__, size);
    struct fat_file * const file = filestr->fatfilep;
    struct bpb * const fat_bpb = FAT_BPB(file->volume);
    if (!fat_bpb)
        return -1;

    int rc;

    if (!size && file->firstcluster)
    {
        /* empty file */
        rc = update_fat_entry(fat_bpb, file->firstcluster, 0);
        if (rc < 0)
            FAT_ERROR(rc * 10 - 2);

        file->firstcluster = 0;
        fat_rewind(filestr);
    }

    if (file->dircluster)
    {
        rc = update_short_entry(fat_bpb, file, size, fatentp);
        if (rc < 0)
            FAT_ERROR(rc * 10 - 3);
    }
    else if (fatentp)
    {
        fat_empty_fat_direntry(fatentp);
    }

#ifdef TEST_FAT
    if (file->firstcluster)
    {
        unsigned long count = 0;
        for (long next = file->firstcluster; next;
             next = get_next_cluster(fat_bpb, next))
        {
            DEBUGF("cluster %ld: %lx\n", count, next);
            count++;
        }

        uint32_t len = count * fat_bpb->bpb_secperclus * SECTOR_SIZE;
        DEBUGF("File is %lu clusters (chainlen=%lu, size=%lu)\n",
               count, len, size );

        if (len > size + fat_bpb->bpb_secperclus * SECTOR_SIZE)
            panicf("Cluster chain is too long\n");

        if (len < size)
            panicf("Cluster chain is too short\n");
    }
#endif /* TEST_FAT */

    rc = 0;
fat_error:
    cache_commit(fat_bpb);
    return rc;
}

void fat_filestr_init(struct fat_filestr *fatstr, struct fat_file *file)
{
    fatstr->fatfilep = file;
    fat_rewind(fatstr);
}

unsigned long fat_query_sectornum(const struct fat_filestr *filestr)
{
    /* return next sector number to be transferred */
    struct bpb * const fat_bpb = FAT_BPB(filestr->fatfilep->volume);
    if (!fat_bpb)
        return INVALID_SECNUM;

    return fat_bpb->bpb_secperclus*filestr->clusternum + filestr->sectornum + 1;
}

/* helper for fat_readwrite */
static long transfer(struct bpb *fat_bpb, unsigned long start, long count,
                     char *buf, bool write)
{
    long rc = 0;

    DEBUGF("%s(s=%lx, c=%lx, wr=%u)\n", __func__,
           start + fat_bpb->startsector, count, write ? 1 : 0);

    if (write)
    {
        unsigned long firstallowed;
#ifdef HAVE_FAT16SUPPORT
        if (fat_bpb->is_fat16)
            firstallowed = fat_bpb->rootdirsector;
        else
#endif /* HAVE_FAT16SUPPORT */
            firstallowed = fat_bpb->firstdatasector;

        if (start < firstallowed)
            panicf("Write %ld before data\n", firstallowed - start);

        if (start + count > fat_bpb->totalsectors)
        {
            panicf("Write %ld after data\n",
                   start + count - fat_bpb->totalsectors);
        }
        else
        {
            rc = storage_write_sectors(IF_MD(fat_bpb->drive,)
                                       start + fat_bpb->startsector, count, buf);
        }
    }
    else
    {
        rc = storage_read_sectors(IF_MD(fat_bpb->drive,)
                                  start + fat_bpb->startsector, count, buf);
    }

    if (rc < 0)
    {
        DEBUGF("Couldn't %s sector %lx (err %ld)\n",
               write ? "write":"read", start, rc);
        return rc;
    }

    return 0;
}

long fat_readwrite(struct fat_filestr *filestr, unsigned long sectorcount,
                   void *buf, bool write)
{
    struct fat_file * const file = filestr->fatfilep;
    struct bpb * const fat_bpb = FAT_BPB(file->volume);
    if (!fat_bpb)
        return -1;

    bool eof = filestr->eof;

    if ((eof && !write) || !sectorcount)
        return 0;

    long rc;

    long          cluster    = filestr->lastcluster;
    unsigned long sector     = filestr->lastsector;
    long          clusternum = filestr->clusternum;
    unsigned long sectornum  = filestr->sectornum;

    DEBUGF("%s(file:%lx,count:0x%lx,buf:%lx,%s)\n", __func__,
           file->firstcluster, sectorcount, (long)buf,
           write ? "write":"read");
    DEBUGF("%s: sec:%lx numsec:%ld eof:%d\n", __func__,
           sector, (long)sectornum, eof ? 1 : 0);

    eof = false;

    if (!sector)
    {
        /* look up first sector of file */
        long newcluster = file->firstcluster;

        if (write && !newcluster)
        {
            /* file is empty; try to allocate its first cluster */
            newcluster = next_write_cluster(fat_bpb, 0);
            file->firstcluster = newcluster;
        }

        if (newcluster)
        {
            cluster = newcluster;
            sector = cluster2sec(fat_bpb, cluster) - 1;

        #ifdef HAVE_FAT16SUPPORT
            if (fat_bpb->is_fat16 && file->firstcluster < 0)
            {
                sector += fat_bpb->rootdirsectornum;
                sectornum = fat_bpb->rootdirsectornum;
            }
        #endif /* HAVE_FAT16SUPPORT */
        }
    }

    if (!sector)
    {
        sectorcount = 0;
        eof = true;
    }

    unsigned long transferred = 0;
    unsigned long count = 0;
    unsigned long last = sector;

    while (transferred + count < sectorcount)
    {
        if (++sectornum >= fat_bpb->bpb_secperclus)
        {
            /* out of sectors in this cluster; get the next cluster */
            long newcluster = write ? next_write_cluster(fat_bpb, cluster) :
                                      get_next_cluster(fat_bpb, cluster);
            if (newcluster)
            {
                cluster = newcluster;
                sector = cluster2sec(fat_bpb, cluster) - 1;
                clusternum++;
                sectornum = 0;

                /* jumped clusters right at start? */
                if (!count)
                    last = sector;
            }
            else
            {
                sectornum--; /* remain in previous position */
                eof = true;
                break;
            }
        }

        /* find sequential sectors and transfer them all at once */
        if (sector != last || count >= FAT_MAX_TRANSFER_SIZE)
        {
            /* not sequential/over limit */
            rc = transfer(fat_bpb, last - count + 1, count, buf, write);
            if (rc < 0)
                FAT_ERROR(rc * 10 - 2);

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
        rc = transfer(fat_bpb, last - count + 1, count, buf, write);
        if (rc < 0)
            FAT_ERROR(rc * 10 - 3);

        transferred += count;
    }

    rc = (eof && write) ? FAT_RC_ENOSPC : (long)transferred;
fat_error:
    filestr->lastcluster = cluster;
    filestr->lastsector  = sector;
    filestr->clusternum  = clusternum;
    filestr->sectornum   = sectornum;
    filestr->eof         = eof;

    if (rc >= 0)
        DEBUGF("Sectors transferred: %ld\n", rc);

    return rc;
}

void fat_rewind(struct fat_filestr *filestr)
{
    /* rewind the file position */
    filestr->lastcluster  = filestr->fatfilep->firstcluster;
    filestr->lastsector   = 0;
    filestr->clusternum   = 0;
    filestr->sectornum    = FAT_FILE_RW_VAL;
    filestr->eof          = false;
}

void fat_seek_to_stream(struct fat_filestr *filestr,
                        const struct fat_filestr *filestr_seek_to)
{
    filestr->lastcluster = filestr_seek_to->lastcluster;
    filestr->lastsector  = filestr_seek_to->lastsector;
    filestr->clusternum  = filestr_seek_to->clusternum;
    filestr->sectornum   = filestr_seek_to->sectornum;
    filestr->eof         = filestr_seek_to->eof;
}

int fat_seek(struct fat_filestr *filestr, unsigned long seeksector)
{
    const struct fat_file * const file = filestr->fatfilep;
    struct bpb * const fat_bpb = FAT_BPB(file->volume);
    if (!fat_bpb)
        return -1;

    int rc;
    long          cluster    = file->firstcluster;
    unsigned long sector     = 0;
    long          clusternum = 0;
    unsigned long sectornum  = FAT_FILE_RW_VAL;

#ifdef HAVE_FAT16SUPPORT
    if (fat_bpb->is_fat16 && cluster < 0) /* FAT16 root dir */
        seeksector += fat_bpb->rootdirsectornum;
#endif /* HAVE_FAT16SUPPORT */

    filestr->eof = false;
    if (seeksector)
    {
        if (cluster == 0)
        {
            DEBUGF("Seeking beyond the end of empty file! "
                   "(sector %lu, cluster %ld)\n", seeksector,
                   seeksector / fat_bpb->bpb_secperclus);
            FAT_ERROR(FAT_SEEK_EOF);
        }

        /* we need to find the sector BEFORE the requested, since
           the file struct stores the last accessed sector */
        seeksector--;
        clusternum = seeksector / fat_bpb->bpb_secperclus;
        sectornum = seeksector % fat_bpb->bpb_secperclus;

        long numclusters = clusternum;

        if (filestr->clusternum && clusternum >= filestr->clusternum)
        {
            /* seek forward from current position */
            cluster = filestr->lastcluster;
            numclusters -= filestr->clusternum;
        }

        for (long i = 0; i < numclusters; i++)
        {
            cluster = get_next_cluster(fat_bpb, cluster);

            if (!cluster)
            {
                DEBUGF("Seeking beyond the end of the file! "
                       "(sector %lu, cluster %ld)\n", seeksector, i);
                FAT_ERROR(FAT_SEEK_EOF);
            }
        }

        sector = cluster2sec(fat_bpb, cluster) + sectornum;
    }

    DEBUGF("%s(%lx, %lx) == %lx, %lx, %lx\n", __func__,
           file->firstcluster, seeksector, cluster, sector, sectornum);

    filestr->lastcluster = cluster;
    filestr->lastsector  = sector;
    filestr->clusternum  = clusternum;
    filestr->sectornum   = sectornum;

    rc = 0;
fat_error:
    return rc;
}

int fat_truncate(const struct fat_filestr *filestr)
{
    DEBUGF("%s(): %lX\n", __func__, filestr->lastcluster);

    struct bpb * const fat_bpb = FAT_BPB(filestr->fatfilep->volume);
    if (!fat_bpb)
        return -1;

    int rc = 1;

    long last = filestr->lastcluster;
    long next = 0;

    /* truncate trailing clusters after the current position */
    if (last)
    {
        next = get_next_cluster(fat_bpb, last);
        int rc2 = update_fat_entry(fat_bpb, last, FAT_EOF_MARK);
        if (rc2 < 0)
            FAT_ERROR(rc2 * 10 - 2);
    }

    int rc2 = free_cluster_chain(fat_bpb, next);
    if (rc2 <= 0)
    {
        DEBUGF("Failed freeing cluster chain\n");
        rc = 0; /* still partial success */
    }

fat_error:
    return rc;
}


/** Directory stream functions **/

int fat_readdir(struct fat_filestr *dirstr, struct fat_dirscan_info *scan,
                struct filestr_cache *cachep, struct fat_direntry *entry)
{
    int rc = 0;

    /* long file names are stored in special entries; each entry holds up to
       13 UTF-16 characters' thus, UTF-8 converted names can be max 255 chars
       (1020 bytes) long, not including the trailing '\0'. */
    struct fatlong_parse_state lnparse;
    fatlong_parse_start(&lnparse);

    scan->entries = 0;

    while (1)
    {
        unsigned int direntry = ++scan->entry;
        if (direntry >= MAX_DIRENTRIES)
        {
            DEBUGF("%s() - Dir is too large (entry %u)\n", __func__,
                   direntry);
            FAT_ERROR(-1);
        }

        unsigned long sector = direntry / DIR_ENTRIES_PER_SECTOR;
        if (cachep->sector != sector)
        {
            if (cachep->sector + 1 != sector)
            {
                /* Nothing cached or sector isn't contiguous */
                int rc2 = fat_seek(dirstr, sector);
                if (rc2 < 0)
                    FAT_ERROR(rc2 * 10 - 2);
            }

            int rc2 = fat_readwrite(dirstr, 1, cachep->buffer, false);
            if (rc2 <= 0)
            {
                if (rc2 == 0)
                    break; /* eof */

                DEBUGF("%s() - Couldn't read dir (err %d)\n", __func__, rc);
                FAT_ERROR(rc2 * 10 - 3);
            }

            cachep->sector = sector;
        }

        unsigned int index = direntry % DIR_ENTRIES_PER_SECTOR;
        union raw_dirent *ent = &((union raw_dirent *)cachep->buffer)[index];

        if (ent->name[0] == 0)
            break;    /* last entry */

        if (ent->name[0] == 0xe5)
        {
            scan->entries = 0;
            continue; /* free entry */
        }

        ++scan->entries;

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
                struct bpb *fat_bpb = FAT_BPB(dirstr->fatfilep->volume);
                if (!fat_bpb)
                    FAT_ERROR(-4);

                dc_lock_cache();

                while (--scan->entry != FAT_DIRSCAN_RW_VAL) /* at beginning? */
                {
                    ent = cache_direntry(fat_bpb, dirstr, scan->entry);

                    /* name[0] == 0 shouldn't happen here but... */
                    if (!ent || ent->name[0] == 0 || ent->name[0] == 0xe5 ||
                        !IS_LDIR_ATTR(ent->ldir_attr))
                        break;
                }

                dc_unlock_cache();

                /* retry it once from the new position */
                scan->entries = 0;
                continue;
            }
        }
        else if (!IS_VOL_ID_ATTR(ent->attr)) /* ignore volume id entry */
        {
            rc = 1;

            if (!fatlong_parse_finish(&lnparse, ent, entry))
            {
                /* the long entries failed to pass all checks or there is
                   just a short entry. */
                DEBUGF("SN-DOS:'%s'", entry->shortname);
                strcpy(entry->name, entry->shortname);
                scan->entries = 1;
                rc = 2; /* name is OEM */
            }

            DEBUGF("LN:\"%s\"", entry->name);
            break;
        }
    } /* end while */

fat_error:
    if (rc <= 0)
    {
        /* error or eod; stay on last good position */
        fat_empty_fat_direntry(entry);
        scan->entry--;
        scan->entries = 0;
    }

    return rc;
}

void fat_rewinddir(struct fat_dirscan_info *scan)
{
    /* rewind the directory scan counter to the beginning */
    scan->entry   = FAT_DIRSCAN_RW_VAL;
    scan->entries = 0;
}


/** Mounting and unmounting functions **/

bool fat_ismounted(IF_MV_NONVOID(int volume))
{
    return !!FAT_BPB(volume);
}

int fat_mount(IF_MV(int volume,) IF_MD(int drive,) unsigned long startsector)
{
    int rc;

    struct bpb * const fat_bpb = &fat_bpbs[IF_MV_VOL(volume)];
    if (fat_bpb->mounted)
        FAT_ERROR(-1); /* double mount */

    /* fill-in basic info first */
    fat_bpb->startsector = startsector;
#ifdef HAVE_MULTIVOLUME
    fat_bpb->volume      = volume;
#endif
#ifdef HAVE_MULTIDRIVE
    fat_bpb->drive       = drive;
#endif

    rc = fat_mount_internal(fat_bpb);
    if (rc < 0)
        FAT_ERROR(rc * 10 - 2);

    /* it worked */
    fat_bpb->mounted = true;

    /* calculate freecount if unset */
    if (fat_bpb->fsinfo.freecount == 0xffffffff)
        fat_recalc_free(IF_MV(fat_bpb->volume));

    DEBUGF("Freecount: %ld\n", (unsigned long)fat_bpb->fsinfo.freecount);
    DEBUGF("Nextfree: 0x%lx\n", (unsigned long)fat_bpb->fsinfo.nextfree);
    DEBUGF("Cluster count: 0x%lx\n", fat_bpb->dataclusters);
    DEBUGF("Sectors per cluster: %lu\n", fat_bpb->bpb_secperclus);
    DEBUGF("FAT sectors: 0x%lx\n", fat_bpb->fatsize);

    rc = 0;
fat_error:
    return rc;
}

int fat_unmount(IF_MV_NONVOID(int volume))
{
    struct bpb * const fat_bpb = FAT_BPB(volume);
    if (!fat_bpb)
        return -1; /* not mounted */

    /* free the entries for this volume */
    cache_discard(IF_MV(fat_bpb));
    fat_bpb->mounted = false;

    return 0;
}


/** Debug screen stuff **/

#ifdef MAX_LOG_SECTOR_SIZE
int fat_get_bytes_per_sector(IF_MV_NONVOID(int volume))
{
    int bytes = 0;

    struct bpb * const fat_bpb = FAT_BPB(volume);
    if (fat_bpb)
        bytes = fat_bpb->bpb_bytspersec;

    return bytes;
}
#endif /* MAX_LOG_SECTOR_SIZE */

unsigned int fat_get_cluster_size(IF_MV_NONVOID(int volume))
{
    unsigned int size = 0;

    struct bpb * const fat_bpb = FAT_BPB(volume);
    if (fat_bpb)
        size = fat_bpb->bpb_secperclus * SECTOR_SIZE;

    return size;
}

void fat_recalc_free(IF_MV_NONVOID(int volume))
{
    struct bpb * const fat_bpb = FAT_BPB(volume);
    if (!fat_bpb)
        return;

    dc_lock_cache();
    fat_recalc_free_internal(fat_bpb);
    dc_unlock_cache();
}

bool fat_size(IF_MV(int volume,) unsigned long *size, unsigned long *free)
{
    struct bpb * const fat_bpb = FAT_BPB(volume);
    if (!fat_bpb)
        return false;

    unsigned long factor = fat_bpb->bpb_secperclus * SECTOR_SIZE / 1024;

    if (size) *size = fat_bpb->dataclusters * factor;
    if (free) *free = fat_bpb->fsinfo.freecount * factor;

    return true;
}


/** Misc. **/

void fat_empty_fat_direntry(struct fat_direntry *entry)
{
    entry->name[0]      = 0;
    entry->shortname[0] = 0;
    entry->attr         = 0;
    entry->crttimetenth = 0;
    entry->crttime      = 0;
    entry->crtdate      = 0;
    entry->lstaccdate   = 0;
    entry->wrttime      = 0;
    entry->wrtdate      = 0;
    entry->filesize     = 0;
    entry->firstcluster = 0;
}

time_t fattime_mktime(uint16_t fatdate, uint16_t fattime)
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

void fat_init(void)
{
    dc_lock_cache();

    /* mark the possible volumes as not mounted */
    for (unsigned int i = 0; i < NUM_VOLUMES; i++)
    {
        dc_discard_all(IF_MV(i));
        fat_bpbs[i].mounted = false;
    }

    dc_unlock_cache();
}
