/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
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
#define RB_FILESYSTEM_OS
#include "config.h"
#include "system.h"
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "file_internal.h"
#include "fileobj_mgr.h"
#include "dircache_redirect.h"
#include "file.h"
#include "string-extra.h"

/**
 * These functions provide a roughly POSIX-compatible file I/O API.
 */

#define FILDES_MAX (MAX_OPEN_FILES+MAX_OPEN_DIRS)

static struct filestr open_streams[FILDES_MAX];

/* check and return a struct filestr* from a file descriptor number */
static struct filestr * get_filestr(int fildes)
{
    struct filestr *str = &open_streams[fildes];

    if ((unsigned int)fildes >= FILDES_MAX)
        str = NULL;
    else if (str->flags & FDO_BUSY)
        return str;

    DEBUGF("fildes %d: bad file number\n", fildes);
    errno = (str && (str->flags & FD_NONEXIST)) ? ENXIO : EBADF;
    return NULL;
}

#define GET_FILESTR(type, fildes) \
    ({                                              \
        file_internal_lock_##type();                \
        struct filestr *_str = get_filestr(fildes); \
        if (_str)                                   \
            FILESTR_LOCK(type, _str);               \
        else                                        \
            file_internal_unlock_##type();          \
        _str;                                       \
    })

/* release the lock on the stream */
#define RELEASE_FILESTR(type, str) \
    ({                                 \
        FILESTR_UNLOCK(type, (str));   \
        file_internal_unlock_##type(); \
    })

/* find a free file descriptor */
static int alloc_filestr(struct filestr **strp)
{
    for (int fildes = 0; fildes < FILDES_MAX; fildes++)
    {
        struct filestr *str = &open_streams[fildes];
        if (!str->flags)
        {
            *strp = str;
            return fildes;
        }
    }

    DEBUGF("Too many files open\n");
    return -1;
}

/* close two files returning the first error of any failures */
static int close_two_files(struct fileinfo *info1p, struct fileinfo *info2p)
{
    int rc = 0;

    /* attempt to close them both regardless */
    if (info1p)
    {
        int rc2 = close_file_internal(info1p);
        if (rc2 < 0)
        {
            rc = rc2;
            DEBUGF("Failed close info1p: %d\n", rc);
        }
    }

    if (info2p)
    {
        int rc2 = close_file_internal(info2p);
        if (rc2 < 0 && rc >= 0)
        {
            rc = rc2;
            DEBUGF("Failed close info2p: %d\n", rc);
        }
    }

    return rc;
}

/* actually do the open gruntwork */
static int open_internal_inner(const char *path, struct filestr *str,
                               int oflag, unsigned int callflags)
{
    int rc;

    if (oflag & O_CREAT)
        callflags |= FF_PARENTINFO;

    struct path_component_info ci;
    struct fileinfo *infop;
    rc = open_file_internal(path, callflags, &ci, &infop);
    if (rc < 0)
    {
        DEBUGF("Open failed: %d\n", rc_file);
        FILE_ERROR_RC(rc);
    }

    if ((ci.attr & ATTR_DIRECTORY) && (oflag & (O_CREAT|O_ACCMODE)))
    {
        DEBUGF("File is a directory\n");
        FILE_ERROR_RC(-EISDIR);
    }

    if (FSI_S_OPENED(rc))
    {
        if (oflag & O_EXCL)
        {
            DEBUGF("File exists\n");
            FILE_ERROR_RC(-EEXIST);
        }

        if (ci.filesize > FILE_SIZE_T_MAX)
        {
            DEBUGF("File too large\n");
            FILE_ERROR_RC(-EOVERFLOW);
        }
    }
    else if (oflag & O_CREAT) /* rc == FSI_S_OPEN_NOENT */
    {
        /* not found; try to create it */
        if (ci.attr & ATTR_DIRECTORY)
        {
            DEBUGF("File is a directory (O_CREAT)\n");
            FILE_ERROR_RC(-EISDIR);
        }

        rc = create_file_internal(&ci.parentinfo, ci.name, ci.namelen,
                                  callflags, &infop);
        if (rc < 0)
            FILE_ERROR_RC(RC);
    }
    else
    {
        DEBUGF("File not found\n");
        FILE_ERROR_RC(-ENOENT);
    }

    rc = open_filestr_internal(infop, callflags, str);
    if (rc < 0)
        FILE_ERROR_RC(RC);

    if (oflag & O_TRUNC)
    {
        rc = __FILEOP(ftruncate)(str, 0);
        if (rc < 0)
        {
            DEBUGF("O_TRUNC failed: %d\n", rc);
            FILE_ERROR_RC(RC);
        }
    }

    return 0;
file_error:
    if (str->flags)
    {
        int rc2 = close_filestr_internal(str);
        if (rc2 < 0 && rc >= 0)
            rc = rc2;
    }

    if (infop)
    {
        int rc2 = close_file_internal(infop);
        if (rc2 < 0 && rc >= 0)
            rc = rc2;
    }

    return rc;
}

/* allocate a file descriptor, if needed, assemble stream flags and open
   a new stream */
int file_fileop_open_filestr(const char *path, int oflag,
                             unsigned int callflags, struct filestr **strp)
{
    int rc;

    if (*strp)
        *strp = NULL;

    struct filestr *str = str;
    int fildes = alloc_filestr(&str);
    if (fildes < 0)
        FILE_ERROR_RC(-EMFILE);

    callflags &= ~FDO_MASK;

    if (oflag & O_ACCMODE)
    {
        callflags |= FD_WRITE;

        if ((oflag & O_ACCMODE) == O_WRONLY)
            callflags |= FD_WRONLY;

        if (oflag & O_APPEND)
            callflags |= FD_APPEND;
    }
    else if (oflag & O_TRUNC)
    {
        /* O_TRUNC requires write mode */
        DEBUGF("O_TRUNC without O_ACCMODE\n");
        FILE_ERROR_RC(-EINVAL);
    }

    /* O_CREAT and O_APPEND are fine without write mode
     * for the former, an empty file is created but no data may be written
     * for the latter, no append will be allowed anyway */
    if (!(oflag & O_CREAT))
    {
        if (oflag & O_EXCL)
            DEBUGF("O_EXCL without O_CREAT\n");

        oflag &= ~O_EXCL; /* result is undefined: we choose "ignore" */
    }

    rc = open_internal_inner(path, str, oflag, callflags);
    if (rc < 0)
        FILE_ERROR_RC(RC);

    if (strp)
        *strp = str;

    rc = fildes;
file_error:
    return rc;
}

int file_fileop_close_filestr(struct filestr *str)
{
    int rc;

    if (!str)
        FILE_ERROR_RC(-EBADF);

    struct fileinfo *infop = str->infop;

    rc = close_filestr_internal(str);
    if (rc < 0)
        FILE_ERROR_RC(RC);

    rc = close_file_internal(infop);
    if (rc < 0)
        FILE_ERROR_RC(RC);

file_error:
    return rc;
}

/* set the file pointer */
off_t file_fileop_lseek(struct filestr *str, off_t offset,
                        int whence, file_size_t limit)
{
    int rc;
    file_size_t pos;

    switch (whence)
    {
    case SEEK_SET:
        if (offset < 0)
            FILE_ERROR_RC(-EINVAL);
        else if ((file_size_t)offset > limit)
            FILE_ERROR_RC(-EOVERFLOW);

        pos = offset;
        break;

    case SEEK_CUR:
        pos = str->offset;
        if (0) {
    case SEEK_END:
        pos = str->infop->size;
        }

        if (offset < 0 && (file_size_t)-offset > pos)
            FILE_ERROR_RC(-EINVAL);
        else if (offset > 0 && (file_size_t)offset > limit - pos)
            FILE_ERROR_RC(-EOVERFLOW);

        pos += offset;
        break;

    default:
        FILE_ERROR_RC(-EINVAL);
    }

    str->offset = pos;

    return pos;

file_error:
    return rc;
}

/* if closing filesystem, strip all write access */
void file_fileop_filesystem_close(void)
{
    for (int fildes = 0; fildes < FILDES_MAX; fildes++)
    {
        struct filestr *str = &open_streams[fildes];
        if (str->flags & FD_WRITE)
        {
            sync_file_internal(str->infop);
            str->flags &= ~FD_WRITE;
        }
    }
}

/** POSIX **/

/* open a file */
int open(const char *path, int oflag)
{
    DEBUGF("%s:path='%s':of=%X\n", __func__, path, (unsigned)oflag);

    int rc;

    file_internal_lock_WRITER();
    rc = file_fileop_open_filestr(path, oflag, 0, NULL);
    file_internal_unlock_WRITER();

    SET_ERRNO_RC(rc, -1);
    return rc;
}

/* create a new file or rewrite an existing one */
int creat(const char *path)
{
    DEBUGF("%s:path='%s'\n", __func__, path);

    int rc;

    file_internal_lock_WRITER();
    rc = file_fileop_open_filestr(path, O_WRONLY|O_CREAT|O_TRUNC, 0, NULL);
    file_internal_unlock_WRITER();

    SET_ERRNO_RC(rc, -1);
    return rc;
}

/* close a file descriptor */
int close(int fildes)
{
    DEBUGF("%s:fd=%d\n", __func__, fildes);

    int rc;

    file_internal_lock_WRITER();

    /* needs to work even if marked "nonexistant" */
    struct filestr *str = &open_streams[fildes];
    if ((unsigned int)fildes >= FILDES_MAX || !str->flags)
    {
        DEBUGF("File not open\n", fildes);
        FILE_ERROR_RC(-EBADF);
    }

    rc = file_fileop_close_filestr(str);

file_error:
    file_internal_unlock_WRITER();

    SET_ERRNO_RC(rc, -1);
    return rc;
}

/* truncate a file to a specified length */
int ftruncate(int fildes, off_t length)
{
    DEBUGF("%s:fd=%d:len=%ld\n", __func__, fildes, (long)length);

    struct filestr * const str = GET_FILESTR(READER, fildes);
    if (!str)
        return -1;

    int rc;

    if (!(str->flags & FD_WRITE))
    {
        DEBUGF("Descriptor is read-only mode\n");
        FILE_ERROR_RC(-EBADF);
    }

    if (length < 0)
    {
        DEBUGF("Length is invalid\n");
        FILE_ERROR_RC(-EINVAL);
    }

    rc = __FILEOP(ftruncate)(str, length);

file_error:
    RELEASE_FILESTR(READER, str);

    SET_ERRNO_RC(rc, -1);
    return rc;
}

/* synchronize changes to a file */
int fsync(int fildes)
{
    DEBUGF("%s:fd=%d\n", __func__, fildes);

    struct filestr * const str = GET_FILESTR(WRITER, fildes);
    if (!str)
        return -1;

    int rc;

    if (!(str->flags & FD_WRITE))
    {
        DEBUGF("Descriptor is read-only mode\n");
        FILE_ERROR_RC(-EINVAL);
    }

    rc = sync_file_internal(str->infop);

file_error:
    RELEASE_FILESTR(WRITER, str);

    SET_ERRNO_RC(rc, -1);
    return rc;
}

/* move the read/write file offset */
off_t lseek(int fildes, off_t offset, int whence)
{
    DEBUGF("%s:fd=%d:ofs=%ld:wh=%d\n", __func__,
           fildes, (long)offset, whence);

    struct filestr * const str = GET_FILESTR(READER, fildes);
    if (!str)
        return -1;

    off_t rc = __FILEOP(lseek)(str, offset, whence);

    RELEASE_FILESTR(READER, str);

    SET_ERRNO_RC(rc, -1);
    return rc;
}

/* read from a file */
ssize_t read(int fildes, void *buf, size_t nbyte)
{
    struct filestr * const str = GET_FILESTR(READER, fildes);
    if (!str)
        return -1;

    ssize_t rc;

    if (!buf)
        FILE_ERROR_RC(-EFAULT);

    if (str->flags & FD_WRONLY)
    {
        DEBUGF("%s:fd=%d:buf=%p:nb=%lu - "
               "descriptor is write-only mode\n",
               __func__, fildes, buf, (unsigned long)nbyte);
        FILE_ERROR_RC(-EBADF);
    }

    rc = __FILEOP(readwrite)(str, buf, nbyte, false);
    if (rc < 0)
        FILE_ERROR_RC(RC);

file_error:
    RELEASE_FILESTR(READER, str);

    SET_ERRNO_RC(rc, -1);
    return rc;
}

/* write on a file */
ssize_t write(int fildes, const void *buf, size_t nbyte)
{
    struct filestr * const str = GET_FILESTR(READER, fildes);
    if (!str)
        return -1;

    ssize_t rc;

    if (!buf)
        FILE_ERROR_RC(-EFAULT);

    if (!(str->flags & FD_WRITE))
    {
        DEBUGF("%s:fd=%d:buf=%p:nb=%lu - "
               "descriptor is read-only mode\n",
               __func__, fildes, buf, (unsigned long)nbyte);
        FILE_ERROR_RC(-EBADF);
    }

    rc = __FILEOP(readwrite)(str, (void *)buf, nbyte, true);
    if (rc < 0)
        FILE_ERROR_RC(RC);

file_error:
    RELEASE_FILESTR(READER, str);

    SET_ERRNO_RC(rc, -1);
    return rc;
}

/* remove a file */
int remove(const char *path)
{
    DEBUGF("%s:path='%s'\n", __func__, path);

    int rc;

    file_internal_lock_WRITER();

    rc = remove_file_internal(path, FF_FILE, NULL);
    if (rc < 0)
        FILE_ERROR_RC(RC);

    rc = 0;
file_error:
    file_internal_unlock_WRITER();

    SET_ERRNO_RC(rc, -1);
    return rc;
}

/* rename a file */
int rename(const char *old, const char *new)
{
    DEBUGF("%s:old='%s':new='%s'\n", __func__, old, new);

    int rc, rmrc = 0;
    struct fileinfo *oldinfop = NULL, *newinfop = NULL;
    struct path_component_info oldci, newci;

    file_internal_lock_WRITER();

    /* open 'old'; it must exist */
    rc = open_file_internal(old, FF_INFO | FF_PARENTINFO, &oldci, &oldinfop);
    if (!FSI_S_OPENED(rc))
    {
        DEBUGF("Failed opening old: %d\n", rc);
        FILE_ERROR_RC(rc == FSI_S_OPEN_NOENT ? -ENOENT : rc);
    }

    /* if 'old' is a directory then 'new' is also required to be one if 'new'
       is to be overwritten */
    const bool are_dirs = oldci.attr & ATTR_DIRECTORY;

    /* open new (may or may not exist) */
    unsigned int callflags = FF_FILE;
    if (are_dirs)
    {
        /* if 'old' is found while parsing the new directory components then
           'new' contains path prefix that names 'old' */
        callflags = FF_DIR | FF_PREFIX;
        newci.prefixp = &oldci.info;
    }

    rc = open_file_internal(new, callflags | FF_PARENTINFO, &newci, &newinfop);
    if (rc < 0)
    {
        DEBUGF("Failed opening new file: %d\n", rc);
        FILE_ERROR_RC(rc);
    }

    /* if the parent is changing then this is a move, not a simple rename */
    switch (compare_fileinfo_internal(&oldci.parentinfo, &newci.parentinfo))
    {
    case CMPFI_EQ:   /* parents are same dir */
        break;
    case CMPFI_LT:   /* parents have same parent */
    case CMPFI_GT:   /* parents have same parent */
    case CMPFI_XDIR: /* parents have different parents */
        if (!(newci.flags & FF_PREFIX))
            break; /* ok */

        DEBUGF("New contains prefix that names old\n");
        FILE_ERROR_RC(-EINVAL);
        break;
    case CMPFI_XDEV: /* on different devices */
        DEBUGF("Cross-device link\n");
        FILE_ERROR_RC(-EXDEV);
        break;
    default:
        DEBUGF("One or both parents are invalid (how?)\n");
        FILE_ERROR_RC(-EIO);
        break;
    }

    bool is_overwrite = false;

    if (newinfop)
    {
        /* new name exists in parent; check if 'old' is overwriting 'new';
           if it's the very same file, then it's just a rename */
        is_overwrite = newinfop != oldinfop;

        if (is_overwrite)
        {
            if (are_dirs)
            {
                /* the directory to be overwritten must be empty */
                rc = test_dir_empty_internal(newinfop);
                if (rc < 0)
                    FILE_ERROR_RC(RC);
            }
        }
        else if (newci.namelen == oldci.namelen &&  /* case-only is ok */
                 !memcmp(newci.name, oldci.name, oldci.namelen))
        {
            DEBUGF("No name change (success)\n");
            rc = 0;
            FILE_ERROR_RC(RC);
        }
    }
    else if (!are_dirs && (newci.attr & ATTR_DIRECTORY))
    {
        /* even if new doesn't exist, canonical path type must match
           (ie. a directory path such as "/foo/bar/" when old names a file) */
        DEBUGF("New path is a directory\n");
        FILE_ERROR_RC(-EISDIR);
    }

    /* first, create the new entry so that there's never a time that the
       victim's data has no reference in the directory tree, that is, until
       everything else first succeeds */
    struct fileentry *entry = STATIC_FILEENTRY;
    strmemcpy(entry->name, newci.name, newci.namelen);
    entry->namelen = newci.namelen;

    rc = __FILEOP(rename)(&newci.parentinfo, oldinfop, entry);
    if (rc < 0)
    {
        DEBUGF("I/O error renaming file: %d\n", rc);
        FILE_ERROR_RC(RC);
    }

    if (is_overwrite)
    {
        /* 'new' would have been assigned its own directory entry and
           succeeded so at this point it is treated like a remove() call
           on the victim which preserves data until the last reference is
           closed */
        rmrc = remove_file_internal(NULL, callflags, newinfop);
        if (rmrc < 0)
            DEBUGF("Failed removing victim: %d\n", rmrc);
    }

    fileop_onrename(&newci.parentinfo, oldinfop, entry);

    rc = 0;
file_error:;
    int rc2 = close_two_files(oldinfop, newinfop);
    if (rc2 < 0 && rc >= 0)
        rc = rc2;

    if (rmrc < 0 && rc >= 0)
        rc = rmrc;

    file_internal_unlock_WRITER();

    SET_ERRNO_RC(rc, -1);
    return rc;
}


/** Extensions **/

/* get the binary size of a file (in bytes) */
off_t filesize(int fildes)
{
    DEBUGF("%s:fd=%d\n", __func__, fildes);

    struct filestr * const str = GET_FILESTR(READER, fildes);
    if (!str)
        return -1;

    off_t rc;
    file_size_t size = str->infop->size;

    if (size > FILE_OFF_T_MAX)
        FILE_ERROR_RC(-EOVERFLOW);

    rc = (off_t)size;
file_error:
    RELEASE_FILESTR(READER, str);

    SET_ERRNO_RC(rc, -1);
    return rc;
}

/* test if two file descriptors refer to the same file */
int fsamefile(int fildes1, int fildes2)
{
    DEBUGF("%s:filedes1=%d:fildes2=%d\n", __func__, fildes1, fildes2);

    int rc = -1;

    struct filestr * const str1 = GET_FILESTR(WRITER, fildes1);
    if (str1)
    {
        struct filestr * const str2 = get_filestr(fildes2);
        if (str2)
            rc = str1->infop == str2->infop ? 1 : 0;

        RELEASE_FILESTR(WRITER, str1);
    }

    return rc;
}

/* tell the relationship of path1 to path2 */
int relate(const char *path1, const char *path2)
{
    /* this is basically what rename() does but reduced to the relationship
       determination */
    DEBUGF("%s:path1='%s':path2='%s'\n", __func__, path1, path2);

    int rc;
    struct fileinfo *info1p = NULL, *info2p = NULL;
    struct path_component_info ci1, ci2;

    file_internal_lock_WRITER();

    rc = open_file_internal(path1, FF_INFO, &ci1, &info1p);
    if (!FSI_S_OPENED(rc))
    {
        DEBUGF("Failed opening path1: %d\n", open1rc);
        FILE_ERROR_RC(rc == FSI_S_OPEN_NOENT ? -ENOENT : rc);
    }

    ci2.prefixp = info1p;
    rc = open_file_internal(path2, FF_PREFIX, &ci2, &info2p);
    if (rc < 0)
    {
        DEBUGF("Failed opening path2: %d\n", rc);
        FILE_ERROR_RC(rc);
    }

    /* path1 existing and path2's final part not can only be a prefix or
       different */
    if (FSI_S_OPENED(rc) && info1p == info2p)
        rc = RELATE_SAME;
    else if (ci2.flags & FF_PREFIX)
        rc = RELATE_PREFIX;
    else
        rc = RELATE_DIFFERENT;

file_error:;
    int rc2 = close_two_files(info1p, info2p);
    if (rc2 < 0 && rc >= 0)
        rc = rc2;

    file_internal_unlock_WRITER();

    SET_ERRNO_RC(rc, -1);
    return rc;
}

/* test file or directory existence */
bool file_exists(const char *path)
{
    DEBUGF("%d:path='%s'\n", __func__, path);

    int rc;

    file_internal_lock_WRITER();
    rc = open_file_internal(path, 0, NULL, NULL);
    file_internal_unlock_WRITER();

    SET_ERRNO_RC(rc);
    return FSI_S_OPENED(rc);
}
