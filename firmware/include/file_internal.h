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
#include "fs_attr.h"
#include "fs_defines.h"
#include "linked_list.h"
#include "mutex.h"
#include "mrsw_lock.h"
#ifdef HAVE_DIRCACHE
#include "dircache.h"
#endif

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
#include "disk_cache.h"
#else
struct iobuf_handle {};
#endif

#define MAX_OPEN_HANDLES (MAX_OPEN_FILES+MAX_OPEN_DIRS)

/* auxilliary attributes - out of the way of regular ATTR_* bits */
#define ATTR_SYSROOT        (0x8000)
#define ATTR_SYSTEM_ROOT    (ATTR_SYSROOT | ATTR_DIRECTORY)
#define ATTR_MOUNT_POINT    (ATTR_VOLUME | ATTR_DIRECTORY)

#define FSI_S_RDDIR_OK      1 /* found and returned an entry */
#define FSI_S_RDDIR_EOD     0 /* no more entries */

#define FSI_S_RDDIR_HAVEENT(_rc) \
    ((_rc) > FSI_S_RDDIR_EOD)

#define FSI_S_OPEN_OK       1 /* stream opened or probe was successful */
#define FSI_S_OPEN_NOENT    0 /* component info was provided; name doesn't
                                 exist in parent but parent does */

/* returns the last deep traceable internal error number (non-reentrant) */
#define __internal_errno \
    (*({ extern int __internal_errno_g; &__internal_errno_g; }))

enum cmpfi_result
{
    CMPFI_LT   = -1, /* info1 compares less than info2 */
    CMPFI_EQ   =  0, /* file info is the same file */
    CMPFI_GT   = +1, /* info1 compares greater than info2 */
    CMPFI_BADF = -2, /* one or more infos cannot make a valid compare */
    CMPFI_XDIR = -3, /* files are in different directories */
    CMPFI_XDEV = -4, /* files are on different devices */
};

#define FSI_CMPFI_INVALID   INT_MIN

#define FSI_S_OPENED(_rc) \
    ((_rc) == FSI_S_OPEN_OK)

/* hook these to the FAT driver for now */
#define __DISKOP(fn) \
    fat_diskop_##fn
#define __VOLOP(fn) \
    fat_volop_##fn
#define __FILEOP(fn) \
    fat_fileop_##fn

union direntry_cvt_t
{
    struct fileentry *fileentry;
    struct dirent    *dirent;
    struct dirinfo   *dirinfo;
} __attribute__((__transparent_union__));

enum
{
    CVTENT_SRC_FILEENTRY = 0x0,
    CVTENT_SRC_DIRENT    = 0x1,
    CVTENT_SRC_MASK      = 0x3,
    CVTENT_DST_DIRENT    = 0x0,
    CVTENT_DST_DIRINFO   = 0x4,
    CVTENT_DST_MASK      = 0xc,
};


/** Common bitflags used throughout **/

/* bitflags used by open files and descriptors */
enum
{
    /* used in descriptor and common */
    FDO_BUSY       = 0x0001,     /* descriptor/object is in use */
    /* only used in individual stream descriptor */
    FD_WRITE       = 0x0002,     /* descriptor has write mode */
    FD_WRONLY      = 0x0004,     /* descriptor is write mode only */
    FD_APPEND      = 0x0008,     /* descriptor is append mode */
    FD_OPEN_MASK   = FD_WRITE|FD_WRONLY|FD_APPEND,
    FD_NONEXIST    = 0x8000,     /* forced closed (not with FDO_BUSY) */
    /* only used as common flags */
    FO_DIRECTORY   = 0x0010,     /* fileobj is a directory */
    FO_SYNC        = 0x0020,     /* file sync is needed */
    FO_OPEN_MASK   = FO_DIRECTORY,
    FDO_MASK       = 0x803f,
    /* bitflags that instruct various 'open' functions how to behave;
     * saved in stream flags (only) but not used by manager */
    FF_ANYTYPE     = 0x00000000, /* succeed if either file or dir (default) */
    FF_FILE        = 0x00010000, /* expect file; accept file only */
    FF_DIR         = 0x00020000, /* expect dir; accept dir only */
    FF_TYPEMASK    = 0x00030000, /* mask of typeflags */
    FF_PREFIX      = 0x00040000, /* detect(ed) prefix of path */
    FF_NOISO       = 0x00080000, /* do not decode ISO filenames to UTF-8 */
    FF_CACHEONLY   = 0x00100000, /* succeed only if in dircache */
    FF_INFO        = 0x00200000, /* return info on self */
    FF_PARENTINFO  = 0x00400000, /* return info on parent */
    FF_SYSROOT     = 0x00800000, /* object is the system root */
    FF_MASK        = 0x00ff0000,
    FF_INFO_MASK   = FF_SYSROOT,
    FF_STR_MASK    = 0,
};

/** Common data structures used throughout **/

/* These get defined in the driver so they see the pointers as their own,
 * e.g.:
 *
 * FS_DRV_STRUCT_COMPLETE(fileinfo, FAT)
 *
 * This way of doing things also prevents every driver file from having to
 * include complete definitions from every other. Only one kind is in use at
 * a time for a given volume or file, so the allocation areas can be shared.
 */
struct bpb;
struct fileentry;
struct fileinfo;
struct filestr;
struct dirscan;

/* Shared fields (this would be cleaner with -fms-extensions) */
#define __struct_bpb_HDR \
    struct {                                                              \
    unsigned char       mounted;     /* 1: mounted, 0: not mounted */     \
    unsigned char       drive;       /* on which physical device  */      \
    unsigned char       volume;      /* on which volume ? */              \
    unsigned long       startsector; /* beginning of partition on disk */ \
    struct ll_head      openfiles;   /* open files on this volume */      \
    struct iocache_bpb  ioc;         /* cache info kept by volume */      \
    }

#define __struct_fileentry_HDR \
    struct {                                                              \
    char                *name;                                            \
    size_t              namelen;                                          \
    }

#define __struct_fileinfo_HDR \
    struct {                                                              \
    struct ll_node      node;        /* list item node (first!) */        \
    uint32_t            flags;       /* F[D]O_* bits of this file */      \
    struct bpb          *bpb;        /* partition information */          \
    file_size_t         size;        /* file size in bytes */             \
    }

#define __struct_filestr_HDR \
    struct {                                                              \
    struct ll_node      node;        /* list item node (first!) */        \
    uint32_t            flags;       /* F[DF]_* bits of this stream */    \
    file_size_t         offset;      /* file pointer */                   \
    struct fileinfo     *infop;      /* base file information */          \
    struct mutex        *mtx;        /* serialization for this stream */  \
    struct iobuf_handle iob;         /* current IO cache buffer handle */ \
    }

#define __struct_dirscan_HDR         /* *placeholder* */

/* 1) define the macros */
#if defined (__FILESYS_STRUCTS_HDR)

/* basic definitions with only the headers */
#undef __FILESYS_STRUCTS_DRV
#undef __FILESYS_STRUCTS_FULL

#define FS_HDR_STRUCT_COMPLETE(__tag__) \
    struct __tag__ {          \
    __struct_##__tag__##_HDR; \
    }

#elif defined (__FILESYS_STRUCTS_DRV)

/* flattened structs for driver code */
#undef __FILESYS_STRUCTS_FULL

/* FS driver sees this as a flat structure of its own fields */
#define FS_DRV_STRUCT_COMPLETE(__tag__, __drvname__) \
    struct __tag__ {                        \
    __struct_##__tag__##_HDR;               \
    __struct_##__tag__##_DRV_##__drvname__; \
    }

/* Actual non-generic definitions done in target's code, not here */

#else /* 1) */

/* full-sized structures */
#define __FILESYS_STRUCTS_FULL

#define FS_FULL_STRUCT_COMPLETE(__tag__) \
    struct __tag__ {          \
    __struct_##__tag__##_HDR; \
    struct __tag__##_drv drv; \
    }

/* add drivers structs here */
#define __FS_STRUCT_DRV_LIST(__tag__) \
    union {                                               \
    __struct_DRV_FAT( __struct_##__tag__##_DRV_FAT fat; ) \
    __struct_DRV_PLT( __struct_##__tag__##_DRV_PLT plt; ) \
    }
#endif /* 1) */

/* 2) include headers that use them but who's definitions are needed below */
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
#include "fat.h"
#endif

/* 2.25) */
#if defined (__FILESYS_STRUCTS_HDR)
/* bpb is needed early for step 2.5) */
FS_HDR_STRUCT_COMPLETE(bpb);
#elif defined (__FILESYS_STRUCTS_DRV)
/* Nothing */
#else /* __FILESYS_STRUCT_FULL */
#ifndef __struct_DRV_FAT
#define __struct_DRV_FAT(...)
#endif
#ifndef __struct_DRV_PLT
#define __struct_DRV_PLT(...)
#endif /* 2) */
struct bpb_drv
{
    __FS_STRUCT_DRV_LIST(bpb);
};

FS_FULL_STRUCT_COMPLETE(bpb);
#endif /* struct definition selection */

/* 3) Actually define the types */
#if defined (__FILESYS_STRUCTS_HDR)
FS_HDR_STRUCT_COMPLETE(fileinfo);
FS_HDR_STRUCT_COMPLETE(filestr);
FS_HDR_STRUCT_COMPLETE(dirscan);
FS_HDR_STRUCT_COMPLETE(fileentry);
#elif defined (__FILESYS_STRUCTS_DRV)
/* Nothing - will be done in driver header */
#else /* __FILESYS_STRUCT_FULL */
/* These are the true, final forms */
struct fileinfo_drv
{
    __FS_STRUCT_DRV_LIST(fileinfo);
#ifdef HAVE_DIRCACHE
    struct dircache_file dc;
#endif
};

struct filestr_drv
{
    __FS_STRUCT_DRV_LIST(filestr);
};

struct dirscan_drv
{
    __FS_STRUCT_DRV_LIST(dirscan);
#ifdef HAVE_DIRCACHE
    struct dircache_file dc;
#endif
};

struct fileentry_drv
{
    __FS_STRUCT_DRV_LIST(fileentry);
#ifdef HAVE_DIRCACHE
    struct dircache_file dc;
#endif
};

FS_FULL_STRUCT_COMPLETE(fileinfo);
FS_FULL_STRUCT_COMPLETE(filestr);
FS_FULL_STRUCT_COMPLETE(dirscan);
FS_FULL_STRUCT_COMPLETE(fileentry);
#endif /* 3) */

static inline void fileinfo_hdr_init(struct bpb *bpb,
                                     size_t size,
                                     struct fileinfo *infop)
{
    infop->flags = 0;
    infop->bpb   = bpb;
    infop->size  = size;
}

static inline void fileinfo_hdr_destroy(struct fileinfo *infop)
{
    infop->flags = 0;
    infop->bpb   = NULL;
}

static inline void filestr_hdr_init(struct fileinfo *infop,
                                    struct filestr *str)
{
    str->flags  = 0;
    str->offset = 0;
    str->infop  = infop;
    iobuf_init(&str->iob);
}

static inline void filestr_hdr_destroy(struct filestr *str)
{
    str->flags = 0;
    str->infop = NULL;
}

static inline void filestr_lock(struct filestr *str)
{
    mutex_lock(str->mtx);
}

static inline void filestr_unlock(struct filestr *str)
{
    mutex_unlock(str->mtx);
}

/* stream lock doesn't have to be used if getting RW lock writer access */
#define FILESTR_WRITER 0
#define FILESTR_READER 1

#define FILESTR_LOCK(type, str) \
    ({ if (FILESTR_##type) filestr_lock(str); })

#define FILESTR_UNLOCK(type, str) \
    ({ if (FILESTR_##type) filestr_unlock(str); })

#define FILEINFO_NEXT(fip) \
    ({ struct fileinfo *__fip = (fip); \
       (struct fileinfo *)__fip->node.next; })

#define FILEINFO_VOL(fip) \
    ({ IF_MV( struct fileinfo *__fip = (fip); ) \
       IF_MV_VOL(__fip->bpb->volume); })

#define FILESTR_VOL(fsp) \
    ({ IF_MV( struct filestr *__fsp = (fsp); ) \
       FILEINFO_VOL(__fsp->infop); })


#ifdef __FILESYS_STRUCTS_FULL
/* These require full-size struct definitions so we hide them for those not
   using them */

/* copy the filesystem file object information in a struct fileinfo */
static inline void fileinfo_copy_fsinfo(struct fileinfo *infop,
                                        const struct fileinfo *srcinfop)
{
    /* infop->flags left alone */
    infop->bpb  = srcinfop->bpb;
    infop->size = srcinfop->size;
    infop->drv  = srcinfop->drv;
}

static inline void fileinfo_copy_fsinfo_f(struct fileinfo *infop,
                                          unsigned int flags,
                                          const struct fileinfo *srcinfop)
{
    infop->flags = flags;
    infop->bpb   = srcinfop->bpb;
    infop->size  = srcinfop->size;
    infop->drv   = srcinfop->drv;
}

/* structure to return detailed information about what you opened */
struct path_component_info
{
    const char   *name;             /* OUT: pointer to name within 'path' */
    size_t       namelen;           /* OUT: length of component within 'path' */
    file_size_t  filesize;          /* OUT: size of the opened file (0 if dir) */
    unsigned int attr;              /* OUT: attributes of this component */
    uint32_t     flags;             /* OUT: flags of this component */
    struct fileinfo info;           /* OUT: base info on file
                                            (FF_INFO) */
    struct fileinfo parentinfo;     /* OUT: base parent directory info
                                            (FF_PARENTINFO) */
    struct fileinfo const *prefixp; /* IN:  base info to check as prefix
                                            (FF_PREFIX) */
};

/* file functions */
int open_file_internal(const char *path,
                       unsigned int callflags,
                       struct path_component_info *compinfo,
                       struct fileinfo **infopp);

int create_file_internal(struct fileinfo *parentinfop,
                         const char *basename,
                         size_t namelen,
                         unsigned int callflags,
                         struct fileinfo **infopp);

int close_file_internal(struct fileinfo *infop);

int remove_file_internal(const char *path,
                         unsigned int callflags,
                         struct fileinfo *infop);

int sync_file_internal(struct fileinfo *infop);

/* stream functions */
int open_filestr_internal(struct fileinfo *infop,
                          unsigned int callflags,
                          struct filestr *str);

int close_filestr_internal(struct filestr *str);

/* directory functions */
int uncached_readdir_internal(struct filestr *dirstr,
                              struct dirscan *scanp,
                              struct dirent *entry);

int test_dir_empty_internal(struct fileinfo *dirinfo);

/* compare two names in the context of the parent */
static inline int compare_name_internal(struct fileinfo *dirinfo,
                                        const char *name1,
                                        const char *name2,
                                        size_t length)
{
    return __FILEOP(compare_name)(dirinfo, name1, name2, length);
}

#endif /* __FILESYS_STRUCTS_FULL */

/** Synchronization used throughout **/

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

#define ERRNO 0 /* maintain errno value */
#define RC    0 /* maintain rc value */

/* sets rc to _rc and jumps to the file_error: label */
#define FILE_ERROR_RC(_rc) \
    ({ if ((_rc) != RC) rc = (_rc); \
       goto file_error; })

/* sets errno to -_nerrno and rc to _rc if _nerrno < 0, else no change */
#define SET_ERRNO_RC(_nerrno, _rc...) \
    ({ if ((_nerrno) < 0) {          \
           errno = -(_nerrno);       \
           if (*#_rc) rc = (_rc +0); \
       } })

/* sets errno to -_nerrno if _nerrno < 0; always returns the value _rc */
#define SET_ERRNO_RC_RETURN(_nerrno, _rc...) \
    ({ if ((_nerrno) < 0) {    \
           errno = -(_nerrno); \
       }                       \
       return _rc; })

/** Misc. stuff **/

/* iterate through all the volumes if volume < 0, else just the given volume */
#define FOR_EACH_VOLUME(volume, i) \
    for (int i = (IF_MV_VOL(volume) >= 0 ? IF_MV_VOL(volume) : 0), \
             _end = (IF_MV_VOL(volume) >= 0 ? i : NUM_VOLUMES-1);  \
         i <= _end; i++)

#define bpb_ismounted_internal(__bpb) \
    ({ struct bpb *___bpb = (__bpb); \
       ___bpb && ___bpb->mounted; })

#define volume_ismounted_internal(volume) \
    bpb_ismounted_internal(get_vol_bpb(volume))

#define STATIC_FILEENTRY \
    ({ extern struct fileentry __fileentry_g; \
       &__fileentry_g; })

/* some reusable helper routines in file.c */
int file_fileop_open_filestr(const char *path,
                             int oflag,
                             unsigned int callflags,
                             struct filestr **strp);

int file_fileop_close_filestr(struct filestr *str);

off_t file_fileop_lseek(struct filestr *str,
                        off_t offset,
                        int whence,
                        file_size_t limit);

void file_fileop_filesystem_close(void);

#define __vol_active(_bpb) \
    ({ struct bpb *__bpb = (_bpb);       \
       extern unsigned int ___vol_active; \
       ___vol_active |= 1u << __bpb->volume; })

#define __vol_clean(_bpb) \
    ({ struct bpb *__bpb = (_bpb);     \
       extern unsigned int ___vol_active; \
       ___vol_active &= ~(1u << __bpb->volume); })

/* one-time init at boot */
void filesystem_init(void) INIT_ATTR;

/* flush and close at shutdown/rolo */
void filesystem_close(void);

#endif /* _FILE_INTERNAL_H_ */
