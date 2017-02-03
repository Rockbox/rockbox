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
#include "file.h"
#include "fileobj_mgr.h"
#include "disk_cache.h"
#include "rb_namespace.h"
#include "string-extra.h"

/**
 * These functions provide a roughly POSIX-compatible file I/O API.
 */

static struct filestr_desc open_streams[MAX_OPEN_FILES];

/* check and return a struct filestr_desc* from a file descriptor number */
static struct filestr_desc * get_filestr(int fildes)
{
    struct filestr_desc *file = &open_streams[fildes];

    if ((unsigned int)fildes >= MAX_OPEN_FILES)
        file = NULL;
    else if (file->stream.flags & FDO_BUSY)
        return file;

    DEBUGF("fildes %d: bad file number\n", fildes);
    errno = EBADF;
    return NULL;
}

#define GET_FILESTR(type, fildes) \
    ({                                                     \
        file_internal_lock_##type();                       \
        struct filestr_desc * _file = get_filestr(fildes); \
        if (_file)                                         \
            FILESTR_LOCK(type, &_file->stream);            \
        else                                               \
            file_internal_unlock_##type();                 \
        _file;                                             \
    })

/* release the lock on the filestr_desc* */
#define RELEASE_FILESTR(type, file) \
    ({                                         \
        FILESTR_UNLOCK(type, &(file)->stream); \
        file_internal_unlock_##type();         \
    })

/* find a free file descriptor */
static int alloc_filestr(struct filestr_desc **filep)
{
    for (int fildes = 0; fildes < MAX_OPEN_FILES; fildes++)
    {
        struct filestr_desc *file = &open_streams[fildes];
        if (!file->stream.flags)
        {
            *filep = file;
            return fildes;
        }
    }

    DEBUGF("Too many files open\n");
    return -1;
}

/* finish with the file and free resources */
static int close_internal(struct filestr_desc *file)
{
    /* call only when holding WRITER lock (updates directory entries) */
    int rc;

    if ((file->stream.flags & (FD_WRITE|FD_NONEXIST)) == FD_WRITE)
    {
        rc = file_fsync(file);
        if (rc < 0)
            FILE_ERROR(ERRNO, rc * 10 - 1);
    }

    rc = 0;
file_error:;
    int rc2 = close_stream_internal(&file->stream);
    if (rc2 < 0 && rc >= 0)
        rc = rc2 * 10 - 2;
    return rc;
}

/* actually do the open gruntwork */
static int open_internal_inner2(const char *path,
                                struct filestr_desc *file,
                                unsigned int callflags
                                __OPEN_MODE_PARM_DECL)
{
    int rc;

    struct path_component_info compinfo;

    if (callflags & FF_CREAT)
        callflags |= FF_PARENTINFO;

    rc = open_stream_internal(path, callflags, &file->stream, &compinfo);
    if (rc < 0)
    {
        DEBUGF("Open failed: %d\n", rc);
        FILE_ERROR_RETURN(ERRNO, rc * 10 - 1);
    }

    bool created = false;

    if (rc > 0)
    {
        if (callflags & FF_EXCL)
        {
            DEBUGF("File exists\n");
            FILE_ERROR(EEXIST, -2);
        }

        if (compinfo.attr & ATTR_DIRECTORY)
        {
            if ((callflags & FD_WRITE) || !(callflags & FF_ANYTYPE))
            {
                DEBUGF("File is a directory\n");
                FILE_ERROR(EISDIR, -3);
            }

            compinfo.filesize = MAX_DIRECTORY_SIZE; /* allow file ops */
        }
    }
    else if (callflags & FF_CREAT)
    {
        if (compinfo.attr & ATTR_DIRECTORY)
        {
            DEBUGF("File is a directory\n");
            FILE_ERROR(EISDIR, -5);
        }

        /* not found; try to create it */

        callflags &= ~FO_TRUNC;
        rc = create_stream_internal(&compinfo.parentinfo, compinfo.name, 
                                    compinfo.length, ATTR_NEW_FILE, callflags,
                                    &file->stream);
        if (rc < 0)
            FILE_ERROR(ERRNO, rc * 10 - 6);

        created = true;
    }
    else
    {
        DEBUGF("File not found\n");
        FILE_ERROR(ENOENT, -7);
    }

    fctrl_io_rewind(&file->stream);
    file->sizep = fileobj_get_sizep(&file->stream);
    file->offset = 0;

    if (!created)
    {
        /* size from storage applies to first stream only otherwise it's
           already up to date */
        const bool first = fileobj_get_flags(&file->stream) & FO_SINGLE;
        if (first)
            *file->sizep = compinfo.filesize;

        if (callflags & FO_TRUNC)
        {
            /* if the file is kind of "big" then free some space now */
            rc = file_ftruncate(file, 0, *file->sizep >= O_TRUNC_THRESH);
            if (rc < 0)
            {
                DEBUGF("O_TRUNC failed: %d\n", rc);
                FILE_ERROR(ERRNO, rc * 10 - 4);
            }
        }
    }

    rc = 0;
file_error:
    if (rc < 0)
        close_stream_internal(&file->stream);

    return rc;
}

/* allocate a file descriptor, if needed, assemble stream flags and open
   a new stream */
static int open_internal_inner1(const char *path, int oflag,
                                unsigned int callflags __OPEN_MODE_PARM_DECL)
{
    DEBUGF("%s(path=\"%s\",oflag=%X,callflags=%X)\n", __func__,
           path, oflag, callflags);

    int rc;

    struct filestr_desc *file;
    int fildes = alloc_filestr(&file);
    if (fildes < 0)
        FILE_ERROR(EMFILE, -1);

    callflags &= ~FDO_MASK;

    if (oflag & O_ACCMODE)
    {
        callflags |= FD_WRITE;

        if ((oflag & O_ACCMODE) == O_WRONLY)
            callflags |= FD_WRONLY;

        if (oflag & O_APPEND)
            callflags |= FD_APPEND;

        if (oflag & O_TRUNC)
            callflags |= FO_TRUNC;
    }
    else if (oflag & O_TRUNC)
    {
        /* O_TRUNC requires write mode */
        DEBUGF("No write mode but have O_TRUNC\n");
        FILE_ERROR(EINVAL, -2);
    }

    /* O_CREAT and O_APPEND are fine without write mode
     * for the former, an empty file is created but no data may be written
     * for the latter, no append will be allowed anyway */
    if (oflag & O_CREAT)
    {
        callflags |= FF_CREAT;

        if (oflag & O_EXCL)
            callflags |= FF_EXCL;
    }

    rc = open_internal_inner2(path, file, callflags __OPEN_MODE_PARM);
    if (rc < 0)
        FILE_ERROR(ERRNO, rc * 10 - 3);

    return fildes;

file_error:
    return rc;
}

static int open_internal_locked(const char *path, int oflag,
                                unsigned int callflags
                                __OPEN_MODE_PARM_DECL)
{
    file_internal_lock_WRITER();
    int rc = open_internal_inner1(path, oflag, callflags __OPEN_MODE_PARM);
    file_internal_unlock_WRITER();
    return rc;
}


/** Internal interface **/

/* open a file without codepage conversion during the directory search;
   required to avoid any reentrancy when opening codepages and when scanning
   directories internally, which could infinitely recurse and would corrupt
   the static data */
int open_noiso_internal(const char *path, int oflag)
{
    return open_internal_locked(path, oflag, FF_ANYTYPE | FF_NOISO);
}

void force_close_writer_internal(struct filestr_base *stream)
{
    /* only we do writers so we know this is our guy */
    close_internal((struct filestr_desc *)stream);
}


/** POSIX **/

/* open a file */
int open(const char *path, int oflag __OPEN_MODE_ARG_DECL)
{
    DEBUGF("open(path=\"%s\",oflag=%X)\n", path, (unsigned)oflag);
    return open_internal_locked(path, oflag, FF_ANYTYPE, __OPEN_MODE_ARG);
}

/* create a new file or rewrite an existing one */
int creat(const char *path __OPEN_MODE_ARG_DECL)
{
    DEBUGF("creat(path=\"%s\")\n", path);
    return open_internal_locked(path, O_WRONLY|O_CREAT|O_TRUNC, FF_ANYTYPE
                                __OPEN_MODE_ARG);
}

/* close a file descriptor */
int close(int fildes)
{
    DEBUGF("close(fd=%d)\n", fildes);

    int rc;

    file_internal_lock_WRITER();

    /* needs to work even if marked "nonexistant" */
    struct filestr_desc *file = &open_streams[fildes];
    if ((unsigned int)fildes >= MAX_OPEN_FILES || !file->stream.flags)
    {
        DEBUGF("filedes %d not open\n", fildes);
        FILE_ERROR(EBADF, -2);
    }

    rc = close_internal(file);
    if (rc < 0)
        FILE_ERROR(ERRNO, rc * 10 - 3);

file_error:
    file_internal_unlock_WRITER();
    return rc;
}

/* truncate a file to a specified length */
int ftruncate(int fildes, off_t length)
{
    DEBUGF("ftruncate(fd=%d,len=%ld)\n", fildes, (long)length);

    struct filestr_desc * const file = GET_FILESTR(READER, fildes);
    if (!file)
        FILE_ERROR_RETURN(ERRNO, -1);

    int rc;

    if (!(file->stream.flags & FD_WRITE))
    {
        DEBUGF("Descriptor is read-only mode\n");
        FILE_ERROR(EBADF, -2);
    }

    if (length < 0)
    {
        DEBUGF("Length %ld is invalid\n", (long)length);
        FILE_ERROR(EINVAL, -3);
    }

    rc = file_ftruncate(file, length, true);
    if (rc < 0)
        FILE_ERROR(ERRNO, rc * 10 - 4);

file_error:
    RELEASE_FILESTR(READER, file);
    return rc;
}

/* synchronize changes to a file */
int fsync(int fildes)
{
    DEBUGF("fsync(fd=%d)\n", fildes);

    struct filestr_desc * const file = GET_FILESTR(WRITER, fildes);
    if (!file)
        FILE_ERROR_RETURN(ERRNO, -1);

    int rc;

    if (!(file->stream.flags & FD_WRITE))
    {
        DEBUGF("Descriptor is read-only mode\n");
        FILE_ERROR(EINVAL, -2);
    }

    rc = file_fsync(file);
    if (rc < 0)
        FILE_ERROR(ERRNO, rc * 10 - 3);

file_error:
    RELEASE_FILESTR(WRITER, file);
    return rc;
}

/* move the read/write file offset */
off_t lseek(int fildes, off_t offset, int whence)
{
    DEBUGF("lseek(fd=%d,ofs=%ld,wh=%d)\n", fildes, (long)offset, whence);

    struct filestr_desc * const file = GET_FILESTR(READER, fildes);
    if (!file)
        FILE_ERROR_RETURN(ERRNO, -1);

    off_t rc = file_lseek(file, offset, whence);
    if (rc < 0)
        FILE_ERROR(ERRNO, rc * 10 - 2);

file_error:
    RELEASE_FILESTR(READER, file);
    return rc;
}

/* read from a file */
ssize_t read(int fildes, void *buf, size_t nbyte)
{
    struct filestr_desc * const file = GET_FILESTR(READER, fildes);
    if (!file)
        FILE_ERROR_RETURN(ERRNO, -1);

    ssize_t rc;

    if (file->stream.flags & FD_WRONLY)
    {
        DEBUGF("read(fd=%d,buf=%p,nb=%lu) - "
               "descriptor is write-only mode\n",
               fildes, buf, (unsigned long)nbyte);
        FILE_ERROR(EBADF, -2);
    }

    rc = file_readwrite(file, buf, nbyte, false);
    if (rc < 0)
        FILE_ERROR(ERRNO, rc * 10 - 3);

file_error:
    RELEASE_FILESTR(READER, file);
    return rc;
}

/* write on a file */
ssize_t write(int fildes, const void *buf, size_t nbyte)
{
    struct filestr_desc * const file = GET_FILESTR(READER, fildes);
    if (!file)
        FILE_ERROR_RETURN(ERRNO, -1);

    ssize_t rc;

    if (!(file->stream.flags & FD_WRITE))
    {
        DEBUGF("write(fd=%d,buf=%p,nb=%lu) - "
               "descriptor is read-only mode\n",
               fildes, buf, (unsigned long)nbyte);
        FILE_ERROR(EBADF, -2);
    }

    rc = file_readwrite(file, (void *)buf, nbyte, true);
    if (rc < 0)
        FILE_ERROR(ERRNO, rc * 10 - 3);

file_error:
    RELEASE_FILESTR(READER, file);
    return rc;
}

/* remove a file */
int remove(const char *path)
{
    DEBUGF("remove(path=\"%s\")\n", path);

    file_internal_lock_WRITER();
    int rc = remove_stream_internal(path, NULL, FF_FILE);
    file_internal_unlock_WRITER();
    return rc;
}

/* rename a file */
int rename(const char *old, const char *new)
{
    DEBUGF("rename(old=\"%s\",new=\"%s\")\n", old, new);

    int rc, open1rc = -1, open2rc = -1;
    struct filestr_base oldstr, newstr;
    struct path_component_info oldinfo, newinfo;

    file_internal_lock_WRITER();

    /* open 'old'; it must exist */
    open1rc = open_stream_internal(old, FF_ANYTYPE | FF_PARENTINFO, &oldstr,
                                   &oldinfo);
    if (open1rc <= 0)
    {
        DEBUGF("Failed opening old: %d\n", open1rc);
        if (open1rc == 0)
            FILE_ERROR(ENOENT, -1);
        else
            FILE_ERROR(ERRNO, open1rc * 10 - 1);
    }

    /* if 'old' is a directory then 'new' is also required to be one if 'new'
       is to be overwritten */
    const bool are_dirs = oldinfo.attr & ATTR_DIRECTORY;

    /* open new (may or may not exist) */
    unsigned int callflags = FF_FILE;
    if (are_dirs)
    {
        /* if 'old' is found while parsing the new directory components then
           'new' contains path prefix that names 'old'; if new and old are in
           the same directory, this tests positive but that is checked later */
        callflags = FF_DIR | FF_CHECKPREFIX;
        newinfo.prefixp = oldstr.infop;
    }

    open2rc = open_stream_internal(new, callflags | FF_PARENTINFO, &newstr,
                                   &newinfo);
    if (open2rc < 0)
    {
        DEBUGF("Failed opening new file: %d\n", open2rc);
        FILE_ERROR(ERRNO, open2rc * 10 - 2);
    }

#ifdef HAVE_MULTIVOLUME
    if (oldinfo.parentinfo.volume != newinfo.parentinfo.volume)
    {
        DEBUGF("Cross-device link\n");
        FILE_ERROR(EXDEV, -3);
    }
#endif /* HAVE_MULTIVOLUME */

    /* if the parent is changing then this is a move, not a simple rename */
    const bool is_move = !fctrl_samefile(&oldinfo.parentinfo.fctrl,
                                         &newinfo.parentinfo.fctrl);
    /* prefix found and moving? */
    if (is_move && (newinfo.attr & ATTR_PREFIX))
    {
        DEBUGF("New contains prefix that names old\n");
        FILE_ERROR(EINVAL, -4);
    }

    const char * const oldname = strmemdupa(oldinfo.name, oldinfo.length);
    const char * const newname = strmemdupa(newinfo.name, newinfo.length);
    bool is_overwrite = false;

    if (open2rc > 0)
    {
        /* new name exists in parent; check if 'old' is overwriting 'new';
           if it's the very same file, then it's just a rename */
        is_overwrite = oldstr.bindp != newstr.bindp;

        if (is_overwrite)
        {
            if (are_dirs)
            {
                /* the directory to be overwritten must be empty */
                rc = test_dir_empty_internal(&newstr);
                if (rc < 0)
                    FILE_ERROR(ERRNO, rc * 10 - 5);
            }
        }
        else if (!strcmp(newname, oldname)) /* case-only is ok */
        {
            DEBUGF("No name change (success)\n");
            rc = 0;
            FILE_ERROR(ERRNO, RC);
        }
    }
    else if (!are_dirs && (newinfo.attr & ATTR_DIRECTORY))
    {
        /* even if new doesn't exist, canonical path type must match
           (ie. a directory path such as "/foo/bar/" when old names a file) */
        DEBUGF("New path is a directory\n");
        FILE_ERROR(EISDIR, -6);
    }

    /* first, create the new entry so that there's never a time that the
       victim's data has no reference in the directory tree, that is, until
       everything else first succeeds */
    struct file_base_info old_fileinfo = *oldstr.infop;
    rc = fctrl_rename_at(&newinfo.parentinfo.fctrl, &oldstr.infop->fctrl,
                         newname);
    if (rc < 0)
    {
        DEBUGF("I/O error renaming file: %d\n", rc);
        FILE_ERROR(rc == FAT_RC_ENOSPC ? ENOSPC : EIO, rc * 10 - 7);
    }

    if (is_overwrite)
    {
        /* 'new' would have been assigned its own directory entry and
           succeeded so at this point it is treated like a remove() call
           on the victim which preserves data until the last reference is
           closed */
        rc = remove_stream_internal(NULL, &newstr, callflags);
        if (rc < 0)
            FILE_ERROR(ERRNO, rc * 10 - 8);
    }

    fileop_onrename_internal(&oldstr, is_move ? &old_fileinfo : NULL,
                             &newinfo.parentinfo, newname);

file_error:
    /* for now, there is nothing to fail upon closing the old stream */
    if (open1rc >= 0)
        close_stream_internal(&oldstr);

    /* the 'new' stream could fail to close cleanly because it became
       impossible to remove its data if this was an overwrite operation */
    if (open2rc >= 0)
    {
        int rc2 = close_stream_internal(&newstr);
        if (rc2 < 0 && rc >= 0)
        {
            DEBUGF("Success but failed closing new: %d\n", rc2);
            rc = rc2 * 10 - 9;
        }
    }

    file_internal_unlock_WRITER();
    return rc;
}


/** Extensions **/

/* get the binary size of a file (in bytes) */
off_t filesize(int fildes)
{
    struct filestr_desc * const file = GET_FILESTR(READER, fildes);
    if (!file)
        FILE_ERROR_RETURN(ERRNO, -1);

    off_t rc;
    file_size_t size = file_filesize(file);

    if (size > FILE_SIZE_MAX)
        FILE_ERROR(EOVERFLOW, -2);

    rc = (off_t)size;
file_error:
    RELEASE_FILESTR(READER, file);
    return rc;
}

/* test if two file descriptors refer to the same file */
int fsamefile(int fildes1, int fildes2)
{
    struct filestr_desc * const file1 = GET_FILESTR(WRITER, fildes1);
    if (!file1)
        FILE_ERROR_RETURN(ERRNO, -1);

    int rc = -2;

    struct filestr_desc * const file2 = get_filestr(fildes2);
    if (file2)
        rc = file1->stream.bindp == file2->stream.bindp ? 1 : 0;

    RELEASE_FILESTR(WRITER, file1);
    return rc;
}

/* tell the relationship of path1 to path2 */
int relate(const char *path1, const char *path2)
{
    /* this is basically what rename() does but reduced to the relationship
       determination */
    DEBUGF("relate(path1=\"%s\",path2=\"%s\")\n", path1, path2);

    int rc, open1rc = -1, open2rc = -1;
    struct filestr_base str1, str2;
    struct path_component_info info1, info2;

    file_internal_lock_WRITER();

    open1rc = open_stream_internal(path1, FF_ANYTYPE, &str1, &info1);
    if (open1rc <= 0)
    {
        DEBUGF("Failed opening path1: %d\n", open1rc);
        if (open1rc < 0)
            FILE_ERROR(ERRNO, open1rc * 10 - 1);
        else
            FILE_ERROR(ENOENT, -1);
    }

    info2.prefixp = str1.infop;
    open2rc = open_stream_internal(path2, FF_ANYTYPE | FF_CHECKPREFIX,
                                   &str2, &info2);
    if (open2rc < 0)
    {
        DEBUGF("Failed opening path2: %d\n", open2rc);
        FILE_ERROR(ERRNO, open2rc * 10 - 2);
    }

    rc = RELATE_DIFFERENT;

    if (open2rc > 0)
    {
        if (str1.bindp == str2.bindp)
            rc = RELATE_SAME;
        else if (info2.attr & ATTR_PREFIX)
            rc = RELATE_PREFIX;
    }
    else /* open2rc == 0 */
    {
        /* path1 existing and path2's final part not can only be a prefix or
           different */
        if (info2.attr & ATTR_PREFIX)
            rc = RELATE_PREFIX;
    }

file_error:
    if (open1rc >= 0)
        close_stream_internal(&str1);

    if (open2rc >= 0)
        close_stream_internal(&str2);

    file_internal_unlock_WRITER();
    return rc;
}

/* test file or directory existence */
bool file_exists(const char *path)
{
    file_internal_lock_WRITER();
    bool rc = test_stream_exists_internal(path, FF_ANYTYPE) > 0;
    file_internal_unlock_WRITER();
    return rc;
}
