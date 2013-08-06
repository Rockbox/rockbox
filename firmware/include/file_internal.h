/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
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
#ifndef _FILE_INTERNAL_H_
#define _FILE_INTERNAL_H_

#include <sys/types.h>
#include <stdlib.h>
#include "mv.h"
#include "linked_list.h"
#include "mutex.h"
#include "mrsw_lock.h"
#include "fs_attr.h"
#include "fat.h"
#ifdef HAVE_DIRCACHE
#include "dircache.h"
#endif

/** Tuneable parameters **/

/* limits for number of open descriptors - if you increase these values, make
   certain that the disk cache has enough available buffers */
#define MAX_OPEN_FILES 11
#define MAX_OPEN_DIRS  12
#define MAX_OPEN_HANDLES (MAX_OPEN_FILES+MAX_OPEN_DIRS)

/* internal functions open streams as well; make sure they don't fail if all
   user descs are busy; this needs to be at least the greatest quantity needed
   at once by all internal functions */
#ifdef HAVE_DIRCACHE
#define AUX_FILEOBJS 3
#else
#define AUX_FILEOBJS 2
#endif

/* number of components statically allocated to handle the vast majority
   of path depths; should maybe be tuned for >= 90th percentile but for now,
   imma just guessing based on something like:
        root + 'Music' + 'Artist' + 'Album' + 'Disc N' + filename */
#define STATIC_PATHCOMP_NUM 6

/* unsigned value that will also hold the off_t range we need without
   overflow */
typedef uint32_t file_size_t;

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 32
#endif

#if _FILE_OFFSET_BITS == 64
/* if we want, we can deal with files up to 2^32-1 bytes-- the full FAT16/32
   range */
#define FILE_SIZE_MAX   (0xffffffffu)
#else
/* file contents and size will be preserved by the APIs so long as ftruncate()
   isn't used; bytes passed 2^31-1 will not accessible nor will writes succeed
   that would extend the file beyond the max for a 32-bit off_t */
#define FILE_SIZE_MAX   (0x7fffffffu)
#endif

/* if file is "large(ish)", then get rid of the contents now rather than
   lazily when the file is synced or closed in order to free-up space */
#define O_TRUNC_THRESH 65536

/* default attributes when creating new files and directories */
#define ATTR_NEW_FILE       (ATTR_ARCHIVE)
#define ATTR_NEW_DIRECTORY  (ATTR_DIRECTORY)


/** File sector cache **/

enum filestr_cache_flags
{
    FSC_DIRTY = 0x1,        /* buffer is dirty (needs writeback) */
    FSC_NEW   = 0x2,        /* buffer is new (never yet written) */
};

struct filestr_cache
{
    uint8_t       *buffer;  /* buffer to hold sector */
    unsigned long sector;   /* file sector that is in buffer */
    unsigned int  flags;    /* FSC_* bits */
};

void file_cache_init(struct filestr_cache *cachep);
void file_cache_reset(struct filestr_cache *cachep);
void file_cache_alloc(struct filestr_cache *cachep);
void file_cache_free(struct filestr_cache *cachep);


/** Common bitflags used throughout **/

/* bitflags used by open files and descriptors */
enum fildes_and_obj_flags
{
    /* used in descriptor and common */
    FDO_BUSY       = 0x0001, /* descriptor/object is in use */
    /* only used in individual stream descriptor */
    FD_WRITE       = 0x0002, /* descriptor has write mode */
    FD_WRONLY      = 0x0004, /* descriptor is write mode only */
    FD_APPEND      = 0x0008, /* descriptor is append mode */
    /* only used as common flags */
    FO_DIRECTORY   = 0x0010, /* fileobj is a directory */
    FO_TRUNC       = 0x0020, /* fileobj is opened to be truncated */
    FO_REMOVED     = 0x0040, /* fileobj was deleted while open */
    FO_SINGLE      = 0x0080, /* fileobj has only one stream open */
    FDO_MASK       = 0x00ff,
    /* bitflags that instruct various 'open' functions how to behave */
    FF_FILE        = 0x0000, /* expect file; accept file only */
    FF_DIR         = 0x0100, /* expect dir; accept dir only */
    FF_ANYTYPE     = 0x0200, /* succeed if either file or dir */
    FF_TYPEMASK    = 0x0300, /* mask of typeflags */
    FF_CREAT       = 0x0400, /* create if file doesn't exist */
    FF_EXCL        = 0x0800, /* fail if creating and file exists */
    FF_CHECKPREFIX = 0x1000, /* detect if file is prefix of path */
    FF_NOISO       = 0x2000, /* do not decode ISO filenames to UTF-8 */
    FF_MASK        = 0x3f00,
    /* special values used in isolation */
    FV_NONEXIST    = 0x8000, /* closed but not freed (unmounted) */
    FV_OPENSYSROOT = 0xc001, /* open sysroot, volume 0 not mounted */
};


/** Common data structures used throughout **/

/* basic file information about its location */
struct file_base_info
{
    union {
#ifdef HAVE_MULTIVOLUME
    int                  volume;  /* file's volume (overlaps fatfile.volume) */
#endif
#if CONFIG_PLATFORM & PLATFORM_NATIVE
    struct fat_file      fatfile; /* FS driver file info */
#endif
    };
#ifdef HAVE_DIRCACHE
    struct dircache_file dcfile;  /* dircache file info */
#endif
};

#define BASEINFO_VOL(infop) \
    IF_MV_VOL((infop)->volume)

/* open files binding item */
struct file_base_binding
{
    struct ll_node        node;   /* list item node (first!) */
    struct file_base_info info;   /* basic file info */
};

#define BASEBINDING_VOL(bindp) \
    BASEINFO_VOL(&(bindp)->info)

/* directory scanning position info */
struct dirscan_info
{
#if CONFIG_PLATFORM & PLATFORM_NATIVE
    struct fat_dirscan_info fatscan; /* FS driver scan info */
#endif
#ifdef HAVE_DIRCACHE
    struct dircache_file    dcscan;  /* dircache scan info */
#endif
};

/* describes the file as an open stream */
struct filestr_base
{
    struct ll_node           node;    /* list item node (first!) */
    uint16_t                 flags;   /* FD_* bits of this stream */
    uint16_t                 unused;  /* not used */
    struct filestr_cache     cache;   /* stream-local cache */
    struct filestr_cache     *cachep; /* the cache in use (local or shared) */
    struct file_base_info    *infop;  /* base file information */
    struct fat_filestr       fatstr;  /* FS driver information */
    struct file_base_binding *bindp;  /* common binding for file/dir */
    struct mutex             *mtx;    /* serialization for this stream */
};

void filestr_base_init(struct filestr_base *stream);
void filestr_base_destroy(struct filestr_base *stream);
void filestr_alloc_cache(struct filestr_base *stream);
void filestr_free_cache(struct filestr_base *stream);
void filestr_assign_cache(struct filestr_base *stream,
                          struct filestr_cache *cachep);
void filestr_copy_cache(struct filestr_base *stream,
                        struct filestr_cache *cachep);
void filestr_discard_cache(struct filestr_base *stream);

/* allocates a cache buffer if needed and returns the cache pointer */
static inline struct filestr_cache *
filestr_get_cache(struct filestr_base *stream)
{
    struct filestr_cache *cachep = stream->cachep;

    if (!cachep->buffer)
        filestr_alloc_cache(stream);

    return cachep;
}

static inline void filestr_lock(struct filestr_base *stream)
{
    mutex_lock(stream->mtx);
}

static inline void filestr_unlock(struct filestr_base *stream)
{
    mutex_unlock(stream->mtx);
}

/* stream lock doesn't have to be used if getting RW lock writer access */
#define FILESTR_WRITER 0
#define FILESTR_READER 1

#define FILESTR_LOCK(type, stream) \
    ({ if (FILESTR_##type) filestr_lock(stream); })

#define FILESTR_UNLOCK(type, stream) \
    ({ if (FILESTR_##type) filestr_unlock(stream); })

#define ATTR_PREFIX (0x8000) /* out of the way of all ATTR_* bits */

/* structure to return detailed information about what you opened */
struct path_component_info
{
    const char   *name;               /* pointer to name within 'path' */
    size_t       length;              /* length of component within 'path' */
    file_size_t  filesize;            /* size of the opened file (0 if dir) */
    unsigned int attr;                /* attributes of this component */
    struct file_base_info *prefixp;   /* base info to check as prefix (IN) */
    struct file_base_info parentinfo; /* parent directory info of file */
};

/* internal path parser result codes */
#define RC_NOT_FOUND   (-5) /* a component was unable to be opened */
#define RC_CONTINUE    (-4) /* continue processing next component */
#define RC_FOUND       (-3) /* the final path component was opened */
#define RC_OPEN_ENOENT (-2) /* last component was unable to be opened */
#define RC_GO_UP       (-1) /* ".." was found (parse_path_component() "up") */
#define RC_PATH_ENDED  ( 0) /* end of path (parse_path_component() "done") */
#define DONE_RC_ADJUST (-3) /* adjustment to turn parse result code into
                               process result code */

int open_stream_internal(const char *path, unsigned int callflags,
                         struct filestr_base *stream,
                         struct path_component_info *compinfo);
int close_stream_internal(struct filestr_base *stream);
int create_stream_internal(struct file_base_info *parentinfop,
                           const char *basename, size_t length,
                           unsigned int attr, unsigned int callflags,
                           struct filestr_base *stream);
int remove_stream_internal(const char *path, struct filestr_base *stream,
                           unsigned int callflags);
int test_stream_exists_internal(const char *path, unsigned int callflags);

/* In file.c for API support */
struct filestr_base * file_get_stream_internal(int fildes, off_t *offsetp);
ssize_t file_open_read_and_close_internal(const char *path, int oflag,
                                          void *buf, size_t nbyte,
                                          off_t offset);

struct dirent;
int uncached_readdir_dirent(struct filestr_base *stream,
                            struct dirscan_info *scanp,
                            struct dirent *entry);
void uncached_rewinddir_dirent(struct dirscan_info *scanp);

int uncached_readdir_internal(struct filestr_base *stream,
                              struct file_base_info *infop,
                              struct fat_direntry *fatent,
                              unsigned int callflags);
void uncached_rewinddir_internal(struct file_base_info *infop);

int test_dir_empty_internal(struct filestr_base *stream);


/** Synchronization used throughout **/

#define FORCE_MUTEX      0
#define FORCE_AUX_WRITER 1

/* acquire the filesystem lock as READER */
static inline void file_internal_lock_READER(void)
{
    extern struct mrsw_lock file_internal_mrsw;
    mrsw_read_acquire(&file_internal_mrsw);
}

/* release the filesystem lock as READER */
static inline void file_internal_unlock_READER(void)
{
    extern struct mrsw_lock file_internal_mrsw;
    mrsw_read_release(&file_internal_mrsw);
}

/* acquire the filesystem lock as WRITER */
static inline void file_internal_lock_WRITER(void)
{
    extern struct mrsw_lock file_internal_mrsw;
    mrsw_write_acquire(&file_internal_mrsw);
}

/* release the filesystem lock as WRITER */
static inline void file_internal_unlock_WRITER(void)
{
    extern struct mrsw_lock file_internal_mrsw;
    mrsw_write_release(&file_internal_mrsw);
}

/* acquire the filesystem lock as READER and lock the auxilliary mutex */
static inline void file_internal_aux_lock(void)
{
    extern struct mutex file_internal_aux_mtx;
    mutex_lock(&file_internal_aux_mtx);
    file_internal_lock_READER();
}

/* unlock the auxilliary mutex and release the filesystem lock as READER */
static inline void file_internal_aux_unlock(void)
{
    file_internal_unlock_READER();
    extern struct mutex file_internal_aux_mtx;
    mutex_unlock(&file_internal_aux_mtx);
}

#define ERRNO 0 /* maintain errno value */
#define RC    0 /* maintain rc value */

/* set errno and rc and proceed to the "file_error:" label */
#define FILE_ERROR(_errno, _rc) \
    ({  __builtin_constant_p(_errno) ?                     \
            ({ if (_errno != ERRNO) errno = (_errno); }) : \
            ({ errno = (_errno); });                       \
        __builtin_constant_p(_rc) ?                        \
            ({ if (_rc != RC) rc = (_rc); }) :             \
            ({ rc = (_rc); });                             \
        goto file_error; })

/* set errno and return a value at the point of invocation */
#define FILE_ERROR_RETURN(_errno, _rc...) \
    ({  __builtin_constant_p(_errno) ?                     \
            ({ if (_errno != ERRNO) errno = (_errno); }) : \
            ({ errno = (_errno); });                       \
        return _rc; })


/** Path parsing helpers **/

/* return a pointer to the character following path separators */
#define GOBBLE_PATH_SEPCH(p) \
    ({ int _c;                          \
       const char *_p = (p);            \
       while ((_c = *_p) == PATH_SEPCH) \
            ++_p;                       \
       _p; })

/* return true if path begins with a root '/' component and is not NULL */
#define PATH_IS_ABSOLUTE(path) \
    ({ const char * const _path = (path); \
       _path && _path[0] == PATH_SEPCH; })

/* return a pointer to the character following a path component which may
   be a separator or the terminating nul */
#define GOBBLE_PATH_COMP(p) \
    ({ int _c;                                \
       const char *_p = (p);                  \
       while ((_c = *_p) && _c != PATH_SEPCH) \
           ++_p;                              \
       _p; })

/* does the character terminate a path component? */
#define IS_COMP_TERMINATOR(c) \
    ({ int _c = (c);               \
       !_c || _c == PATH_SEPCH; })


/** Misc. **/

/* duplicate a string with on the stack with alloca() */
#define STRDUP_ALLOCA(src, length) \
    ({ size_t _l = (length);                   \
       char * _s = alloca(_l + 1);             \
       *(char *)mempcpy(_s, (src), _l) = '\0'; \
       _s; })

/* is the given pointer "p" inside the bounds of "array"? */
#define PTR_IN_ARRAY(array, p, numelements) \
    ((uintptr_t)(p) - (uintptr_t)(array) < \
     (uintptr_t)(numelements)*sizeof ((array)[0]))

/* iterate through all the volumes if volume < 0, else just the given volume */
#define FOR_EACH_VOLUME(volume, i) \
    for (int i = (IF_MV_VOL(volume) >= 0 ? IF_MV_VOL(volume) : 0), \
             _end = (IF_MV_VOL(volume) >= 0 ? i : NUM_VOLUMES-1);  \
         i <= _end; i++)


/** Misc. stuff **/

/* return a pointer to the static struct fat_direntry */
static inline struct fat_direntry *get_dir_fatent(void)
{
    extern struct fat_direntry dir_fatent;
    return &dir_fatent;
}

#ifdef HAVE_DIRCACHE
void empty_dirent(struct dirent *entry);
void fat_direntry_dirinfo(struct dirinfo *dirinfo,
                          const struct fat_direntry *fatent);
void dir_fatent_dirinfo(struct dirinfo *dirinfo);
#endif /* HAVE_DIRCACHE */

void file_internal_init(void) INIT_ATTR;

#endif /* _FILE_INTERNAL_H_ */
