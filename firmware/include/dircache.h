/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Miika Pekkarinen
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
#ifndef _DIRCACHE_H
#define _DIRCACHE_H

#include "mv.h"
#include <string.h>    /* size_t */
#include <sys/types.h> /* ssize_t */

#ifdef HAVE_DIRCACHE

/****************************************************************************
 ** Configurable values
 **/

#if 0
/* enable dumping code */
#define DIRCACHE_DUMPSTER
#define DIRCACHE_DUMPSTER_BIN   "/dircache_dump.bin"
#define DIRCACHE_DUMPSTER_CSV   "/dircache_dump.csv"
#endif

/* dircache builds won't search below this but will work down to this point
   while below it the cache will just pass requests through to the storage;
   the limiting factor is the scanning thread stack size, not the
   implementation -- tune the two together */
#define DIRCACHE_MAX_DEPTH  15
#define DIRCACHE_STACK_SIZE (DEFAULT_STACK_SIZE + 0x200)

/* memory buffer constants that control allocation */
#define DIRCACHE_RESERVE (1024*64)     /* 64 KB - new entry slack */
#define DIRCACHE_MIN     (1024*1024*1) /* 1 MB - provision min size */
#define DIRCACHE_LIMIT   (1024*1024*6) /* 6 MB - provision max size */

/* make it easy to change serialnumber size without modifying anything else;
   32 bits allows 21845 builds before wrapping in a 6MB cache that is filled
   exclusively with entries and nothing else (32 byte entries), making that
   figure pessimistic */
typedef uint32_t dc_serial_t;

/* these should agree with size of dc_serial_t */
#define DC_SERHASH_START        0xffffffff

/* I was originally using FNV hash but decided this is probably okay
   (for now) */
#define dc_hash_serialnum(s, h) \
    ({ dc_serial_t __x = (s); crc_32(&(__x), sizeof(dc_serial_t), (h)); })
#define DC_SERIAL_FMT           "0x%08lX"

/**
 ****************************************************************************/

#define IF_DIRCACHE(...) __VA_ARGS__
#define IFN_DIRCACHE(...)

#if CONFIG_PLATFORM & PLATFORM_NATIVE
/* native dircache is lower-level than on a hosted target */
#define DIRCACHE_NATIVE
#endif

struct dircache_file
{
    int         idx;        /* this file's cache index */
    dc_serial_t serialnum;  /* this file's serial number */
};

enum dircache_status
{
    DIRCACHE_IDLE     = 0,  /* no volume is initialized */
    DIRCACHE_SCANNING = 1,  /* dircache is scanning a volume */
    DIRCACHE_READY    = 2,  /* dircache is ready to be used */
};

/** Dircache control **/
void dircache_wait(void);
void dircache_suspend(void);
int dircache_resume(void);
int dircache_enable(void);
void dircache_disable(void);
void dircache_free_buffer(void);

/** Volume mounting **/
void dircache_mount(void); /* always tries building everything it can */
void dircache_unmount(IF_MV_NONVOID(int volume));


/** File API service functions **/

/* avoid forcing #include of file_internal.h, fat.h and dir.h */
struct filestr_base;
struct file_base_info;
struct file_base_binding;
struct dirent;
struct dirscan_info;
struct dirinfo_native;

int dircache_readdir_dirent(struct filestr_base *stream,
                            struct dirscan_info *scanp,
                            struct dirent *entry);
void dircache_rewinddir_dirent(struct dirscan_info *scanp);

#ifdef DIRCACHE_NATIVE
struct fat_direntry;
int dircache_readdir_internal(struct filestr_base *stream,
                              struct file_base_info *infop,
                              struct fat_direntry *fatent);
void dircache_rewinddir_internal(struct file_base_info *info);
#endif /* DIRCACHE_NATIVE */


/** Dircache live updating **/

void dircache_get_rootinfo(struct file_base_info *infop);
void dircache_bind_file(struct file_base_binding *bindp);
void dircache_unbind_file(struct file_base_binding *bindp);
void dircache_fileop_create(struct file_base_info *dirinfop,
                            struct file_base_binding *bindp,
                            const char *basename,
                            const struct dirinfo_native *dinp);
void dircache_fileop_rename(struct file_base_info *dirinfop,
                            struct file_base_binding *bindp,
                            const char *basename);
void dircache_fileop_remove(struct file_base_binding *bindp);
void dircache_fileop_sync(struct file_base_binding *infop,
                          const struct dirinfo_native *dinp);


/** Dircache paths, files and shortcuts **/
struct dircache_fileref
{
    struct dircache_file dcfile;
    dc_serial_t          serialhash; /* Hash of serialnumbers to root */
};

void dircache_fileref_init(struct dircache_fileref *dcfrefp);
ssize_t dircache_get_fileref_path(const struct dircache_fileref *dcfrefp,
                                  char *buf, size_t size);

/* Bitflags for dircache_search() */
enum dircache_search_flags
{
    DCS_FILEREF        = 0x01, /* Check fileref existence and serial number */
    _DCS_VERIFY_FLAG   = 0x02, /* Internal: Only valid with DCS_FILEREF */
    DCS_FILEREF_VERIFY = 0x03, /* Do DCS_FILEREF check + verify serial hash */
    DCS_CACHED_PATH    = 0x04, /* Check only cache for provided path */
    _DCS_STORAGE_FLAG  = 0x08, /* Internal: Only valid with DCS_CACHED_PATH */
    DCS_STORAGE_PATH   = 0x0c, /* Read-through if needed for provided path */
    DCS_UPDATE_FILEREF = 0x10, /* If fileref is not valid but path is found or
                                  searching a path, update the reference
                                  information */
};

int dircache_search(unsigned int flags, struct dircache_fileref *dcfrefp,
                    const char *path);

int dircache_fileref_cmp(const struct dircache_fileref *dcfrefp1,
                         const struct dircache_fileref *dcfrefp2);


/** Debug screen/info stuff **/

struct dircache_info
{
    enum dircache_status status; /* current composite status value */
    const char   *statusdesc;    /* pointer to string describing 'status' */
    size_t       last_size;      /* cache size after last build */
    size_t       size;           /* total size of entries (with holes) */
    size_t       sizeused;       /* bytes of 'size' actually utilized */
    size_t       size_limit;     /* maximum possible size */
    size_t       reserve;        /* size of reserve area */
    size_t       reserve_used;   /* amount of reserve used */
    unsigned int entry_count;    /* number of cache entries */
    long         build_ticks;    /* total time used to build cache */
};

void dircache_get_info(struct dircache_info *info);
#ifdef DIRCACHE_DUMPSTER
void dircache_dump(void);
#endif /* DIRCACHE_DUMPSTER */


/** Misc. stuff **/
void dircache_dcfile_init(struct dircache_file *dcfilep);

#ifdef HAVE_EEPROM_SETTINGS
int dircache_load(void);
int dircache_save(void);
#endif /* HAVE_EEPROM_SETTINGS */

void dircache_init(size_t last_size) INIT_ATTR;

#else /* !HAVE_DIRCACHE */

#define IF_DIRCACHE(...)
#define IFN_DIRCACHE(...) __VA_ARGS__

#endif /* HAVE_DIRCACHE */

#endif /* _DIRCACHE_H */
