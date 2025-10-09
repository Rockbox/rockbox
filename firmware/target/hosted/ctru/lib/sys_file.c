/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Mauricio G.
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
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include "file.h"
#include "debug.h"
#include "string-extra.h"
#include "fs_defines.h"
#include "sys_file.h"

/* This file is based on firmware/common/file.c */

/* Define LOGF_ENABLE to enable logf output in this file */
// #define LOGF_ENABLE
#include "logf.h"

/**
 * These functions provide a roughly POSIX-compatible file I/O API.
 * Important: the bufferio I/O library (bfile) used in the 3ds does not work 
 * with write-only files due to an internal limitation.
 * So all files will be opened with the read flag by default.
 */

/* structure used for open file descriptors */
static struct filestr_desc
{
    struct filestr_base stream; /* basic stream info (first!) */
    file_size_t         offset; /* current offset for stream */
    u64                 *sizep; /* shortcut to file size in fileobj */
} open_streams[MAX_OPEN_FILES] =
{
    [0 ... MAX_OPEN_FILES-1] = { .stream = { .cache = nil, .flags = 0 } }
};

extern FS_Archive sdmcArchive;

/* check and return a struct filestr_desc* from a file descriptor number */
static struct filestr_desc * get_filestr(int fildes)
{
    struct filestr_desc *file = &open_streams[fildes];

    if ((unsigned int)fildes >= MAX_OPEN_FILES)
        file = NULL;
    else if (file->stream.cache != nil)
        return file;

    logf("fildes %d: bad file number\n", fildes);
    errno = (file && (file->stream.cache == nil)) ? ENXIO : EBADF;
    return NULL;
}

#define GET_FILESTR(type, fildes) \
    ({                                                     \
        file_internal_lock_##type();                       \
        struct filestr_desc * _file = get_filestr(fildes); \
        if (_file)                                         \
            FILESTR_LOCK(type, &_file->stream);            \
        else   {                                            \
            file_internal_unlock_##type();                 \
  }\
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
        if (file->stream.cache == nil)
        {
            *filep = file;
            return fildes;
        }
    }

    logf("Too many files open\n");
    return -1;
}

/* check for file existence */
int test_stream_exists_internal(const char *path)
{
    int rc;
    bool is_dir = false;

    Handle handle;
    Result res = FSUSER_OpenFile(&handle,
	                         sdmcArchive,
	                         fsMakePath(PATH_ASCII, path),
	                         FS_OPEN_READ,
	                         0);
    if (R_FAILED(res)) {
        /* not a file, try to open a directory */
        res = FSUSER_OpenDirectory(&handle,
                                   sdmcArchive,
                                   fsMakePath(PATH_ASCII, path));
        if (R_FAILED(res)) {
            logf("File does not exist\n");
            FILE_ERROR(ERRNO, -1);
        }

        is_dir = true;
    }

    rc = 1;
file_error:
    if (handle > 0) {
        if (is_dir)
            FSDIR_Close(handle);
        else
            FSFILE_Close(handle);
    }

    return rc;
}

/* set the file pointer */
static off_t lseek_internal(struct filestr_desc *file, off_t offset,
                            int whence)
{
    off_t rc;
    off_t pos;

    off_t size = MIN(*file->sizep, FILE_SIZE_MAX);
    off_t file_offset = AtomicLoad(&file->offset);

    switch (whence)
    {
    case SEEK_SET:
        if (offset < 0 || (off_t)offset > size)
            FILE_ERROR(EINVAL, -1);

        pos = offset;
        break;

    case SEEK_CUR:
        if ((offset < 0 && (off_t)-offset > file_offset) ||
            (offset > 0 && (off_t)offset > size - file_offset))
            FILE_ERROR(EINVAL, -1);

        pos = file_offset + offset;
        break;

    case SEEK_END:
        if (offset > 0 || (off_t)-offset > size)
            FILE_ERROR(EINVAL, -1);

        pos = size + offset;
        break;

    default:
        FILE_ERROR(EINVAL, -1);
    }

    AtomicSwap(&file->offset, pos);

    return pos;
file_error:
    return rc;
}

/* read from or write to the file; back end to read() and write() */
static ssize_t readwrite(struct filestr_desc *file, void *buf, size_t nbyte,
                         bool write)
{
#ifndef LOGF_ENABLE /* wipes out log before you can save it */
    /* DEBUGF("readwrite(%p,%lx,%lu,%s)\n",
           file, (long)buf, (unsigned long)nbyte, write ? "write" : "read"); */
#endif

    const file_size_t size = *file->sizep;
    size_t filerem;

    if (write)
    {
        /* if opened in append mode, move pointer to end */
        if (file->stream.flags & O_APPEND)
            AtomicSwap(&file->offset, MIN(size, FILE_SIZE_MAX));

        filerem = FILE_SIZE_MAX - AtomicLoad(&file->offset);
    }
    else
    {
        /* limit to maximum possible offset (EOF or FILE_SIZE_MAX) */
        filerem = MIN(size, FILE_SIZE_MAX) - AtomicLoad(&file->offset);
    }

    if (nbyte > filerem)
    {
        nbyte = filerem;
        if (nbyte > 0)
            {}
        else if (write)
            FILE_ERROR_RETURN(EFBIG, -1);     /* would get too large */
        else if (AtomicLoad(&file->offset) >= FILE_SIZE_MAX)
            FILE_ERROR_RETURN(EOVERFLOW, -2); /* can't read here */
    }

    if (nbyte == 0)
        return 0;

    int rc = 0;
    int_error_t n_err;

    if (write)
        n_err = PagerWriteAt(file->stream.cache, (u8 *) buf, nbyte, AtomicLoad(&file->offset));
    else
        n_err = PagerReadAt(file->stream.cache, (u8 *) buf, nbyte, AtomicLoad(&file->offset));

    if ((n_err.err != nil) && strcmp(n_err.err, "io.EOF")) {
        FILE_ERROR(ERRNO, -3); 
    }

file_error:;
#ifdef DEBUG
    if (errno == ENOSPC)
        logf("No space left on device\n");
#endif

    size_t done = n_err.n;
    if (done)
    {
        /* error or not, update the file offset and size if anything was
           transferred */
        AtomicAdd(&file->offset, done);
#ifndef LOGF_ENABLE /* wipes out log before you can save it */
        /* DEBUGF("file offset: %lld\n", file->offset); */
#endif
        /* adjust file size to length written */
        if (write && AtomicLoad(&file->offset) > size)
            *file->sizep = AtomicLoad(&file->offset);

        return done;
    }

    return rc;
}

/* initialize the base descriptor */
static void filestr_base_init(struct filestr_base *stream)
{
    stream->cache = nil;
    stream->handle = 0;
    stream->size = 0;
    LightLock_Init(&stream->mtx);
}

int open_internal_inner2(Handle *handle, const char *path, u32 openFlags, u32 attributes)
{
    int rc;
    Result res = FSUSER_OpenFile(handle,
	                         sdmcArchive,
	                         fsMakePath(PATH_ASCII, path),
	                         openFlags,
	                         attributes);
    if (R_FAILED(res)) {
       FILE_ERROR(ERRNO, -1);
    }

    rc = 1;
file_error:
    return rc;
}

static int open_internal_inner1(const char *path, int oflag)
{
    int rc;
    struct filestr_desc *file;
    int fildes = alloc_filestr(&file);
    if (fildes < 0)
        FILE_ERROR_RETURN(EMFILE, -1);

    u32 openFlags = 0, attributes = 0;

    /* open for reading by default */
    openFlags = FS_OPEN_READ;

    if (oflag & O_ACCMODE)
    {
        if ((oflag & O_ACCMODE) == O_RDONLY) {
            attributes |= FS_ATTRIBUTE_READ_ONLY;
        }
        if ((oflag & O_ACCMODE) == O_WRONLY) {
            openFlags |= FS_OPEN_WRITE;
        }
        if ((oflag & O_ACCMODE) == O_RDWR) {
            openFlags |= FS_OPEN_WRITE;
        }
    }
    else if (oflag & O_TRUNC)
    {
        /* O_TRUNC requires write mode */
        logf("No write mode but have O_TRUNC\n");
        FILE_ERROR(EINVAL, -2);
    }

    /* O_CREAT and O_APPEND are fine without write mode
     * for the former, an empty file is created but no data may be written
     * for the latter, no append will be allowed anyway */
    if (!(oflag & O_CREAT))
        oflag &= ~O_EXCL; /* result is undefined: we choose "ignore" */

    filestr_base_init(&file->stream);
    rc = open_internal_inner2(&file->stream.handle, path, openFlags, attributes);

    if (rc > 0) {
        if (oflag & O_EXCL)
        {
            logf("File exists\n");
            FILE_ERROR(EEXIST, -4);
        }
    }
    else if (oflag & O_CREAT)
    {
        /* not found; try to create it */
        openFlags |= FS_OPEN_CREATE;
        rc = open_internal_inner2(&file->stream.handle, path, openFlags, attributes);
        if (rc < 0)
            FILE_ERROR(ERRNO, rc * 10 - 6);
    }
    else
    {
        logf("File not found\n");
        FILE_ERROR(ENOENT, -5);
    }

    /* truncate file if requested */
    if (oflag & O_TRUNC) {
        Result res = FSFILE_SetSize(file->stream.handle, 0);
        if (R_FAILED(res)) {
            FILE_ERROR(ERRNO, -6);
        }
    }

    /* we need to set file size here, or else lseek
       will fail if no read or write has been done */
    u64 size = 0;
    Result res = FSFILE_GetSize(file->stream.handle, &size);
    if (R_FAILED(res)) {
        FILE_ERROR(ERRNO, -8);
    }

    int pageSize = 4096;            /* 4096 bytes */
    int bufferSize = 512 * 1024;    /* 512 kB */

    /* streamed file formats like flac and mp3 need very large page
       sizes to avoid stuttering */
    if (((oflag & O_ACCMODE) == O_RDONLY) && (size > 0x200000)) {
        /* printf("open(%s)_BIG_pageSize\n", path); */
        pageSize = 32 * 1024;
        bufferSize = MIN(size, defaultBufferSize);
    }

    file->stream.cache = NewPagerSize(file->stream.handle,
                                      pageSize,
                                      bufferSize);
    if (file->stream.cache == nil) {
        FILE_ERROR(ERRNO, -7);
    }

    file->stream.flags = oflag;
    file->stream.size = size;
    file->sizep = &file->stream.size;
    AtomicSwap(&file->offset, 0);

    /* we will use file path to implement ctru_fsamefile function */
    strcpy(file->stream.path, path);

    return fildes;

file_error:
    if (fildes >= 0) {
        if (file->stream.cache != nil) {
            PagerFlush(file->stream.cache);
            PagerClear(file->stream.cache);
            file->stream.cache = nil;
        }
    
        FSFILE_Close(file->stream.handle);
        file->stream.handle = 0;
    }

    return rc;
}

static int open_internal_locked(const char *path, int oflag)
{
    file_internal_lock_WRITER();
    int rc = open_internal_inner1(path, oflag);
    file_internal_unlock_WRITER();
    return rc;
}

int ctru_open(const char *path, int oflag, ...)
{
    logf("open(path=\"%s\",oflag=%X)\n", path, (unsigned)oflag);
    return open_internal_locked(path, oflag);
}

int ctru_creat(const char *path, mode_t mode)
{
    logf("creat(path=\"%s\")\n", path);
    return ctru_open(path, O_WRONLY|O_CREAT|O_TRUNC, mode);
}

int ctru_close(int fildes)
{
    logf("close(fd=%d)\n", fildes);

    int rc;

    file_internal_lock_WRITER();

    /* needs to work even if marked "nonexistant" */
    struct filestr_desc *file = &open_streams[fildes];
    if ((unsigned int)fildes >= MAX_OPEN_FILES || (file->stream.cache == nil))
    {
        logf("filedes %d not open\n", fildes);
        FILE_ERROR(EBADF, -2);
    }

    if (file->stream.cache != nil) {
        PagerFlush(file->stream.cache);
        PagerClear(file->stream.cache);
        file->stream.cache = nil;
    }
    
    FSFILE_Close(file->stream.handle);
    file->stream.handle = 0;
    file->stream.path[0] = '\0';

    rc = 0;
file_error:
    file_internal_unlock_WRITER();
    return rc;
}

/* truncate a file to a specified length */
int ctru_ftruncate(int fildes, off_t length)
{
    logf("ftruncate(fd=%d,len=%ld)\n", fildes, (long)length);

    struct filestr_desc * const file = GET_FILESTR(READER, fildes);
    if (!file)
        FILE_ERROR_RETURN(ERRNO, -1);

    int rc;

    if (file->stream.flags & O_RDONLY)
    {
        logf("Descriptor is read-only mode\n");
        FILE_ERROR(EBADF, -2);
    }

    if (length < 0)
    {
        logf("Length %ld is invalid\n", (long)length);
        FILE_ERROR(EINVAL, -3);
    }

    file_error_t err = PagerTruncate(file->stream.cache, length);
    if (err) {
        FILE_ERROR(ERRNO, -11);
    }

    *file->sizep = length;

    rc = 0;
file_error:
    RELEASE_FILESTR(READER, file);
    return rc;
}

/* synchronize changes to a file */
int ctru_fsync(int fildes)
{
    logf("fsync(fd=%d)\n", fildes);

    struct filestr_desc * const file = GET_FILESTR(WRITER, fildes);
    if (!file)
        FILE_ERROR_RETURN(ERRNO, -1);

    int rc;

    if (file->stream.flags & O_RDONLY)
    {
        logf("Descriptor is read-only mode\n");
        FILE_ERROR(EINVAL, -2);
    }

    /* flush all pending changes to disk */
    file_error_t err = PagerFlush(file->stream.cache);
    if (err != nil) {
        FILE_ERROR(ERRNO, -3);
    }

    rc = 0;
file_error:
    RELEASE_FILESTR(WRITER, file);
    return rc;
}

/* move the read/write file offset */
off_t ctru_lseek(int fildes, off_t offset, int whence)
{
#ifndef LOGF_ENABLE /* wipes out log before you can save it */
    /* DEBUGF("lseek(fd=%d,ofs=%ld,wh=%d)\n", fildes, (long)offset, whence); */
#endif
    struct filestr_desc * const file = GET_FILESTR(READER, fildes);
    if (!file)
        FILE_ERROR_RETURN(ERRNO, -1);

    off_t rc = lseek_internal(file, offset, whence);
    if (rc < 0)
        FILE_ERROR(ERRNO, rc * 10 - 2);

file_error:
    RELEASE_FILESTR(READER, file);
    return rc;
}

/* read from a file */
ssize_t ctru_read(int fildes, void *buf, size_t nbyte)
{
    struct filestr_desc * const file = GET_FILESTR(READER, fildes);
    if (!file)
        FILE_ERROR_RETURN(ERRNO, -1);

    ssize_t rc;

    if (file->stream.flags & O_WRONLY)
    {
        logf("read(fd=%d,buf=%p,nb=%lu) - "
               "descriptor is write-only mode\n",
               fildes, buf, (unsigned long)nbyte);
        FILE_ERROR(EBADF, -2);
    }

    rc = readwrite(file, buf, nbyte, false);
    if (rc < 0)
        FILE_ERROR(ERRNO, rc * 10 - 3);

file_error:
    RELEASE_FILESTR(READER, file);
    return rc;
}

/* write on a file */
ssize_t ctru_write(int fildes, const void *buf, size_t nbyte)
{
    struct filestr_desc * const file = GET_FILESTR(READER, fildes);
    if (!file)
        FILE_ERROR_RETURN(ERRNO, -1);

    ssize_t rc;

    if (file->stream.flags & O_RDONLY)
    {
        logf("write(fd=%d,buf=%p,nb=%lu) - "
               "descriptor is read-only mode\n",
               fildes, buf, (unsigned long)nbyte);
        FILE_ERROR(EBADF, -2);
    }

    rc = readwrite(file, (void *)buf, nbyte, true);
    if (rc < 0)
        FILE_ERROR(ERRNO, rc * 10 - 3);

file_error:
    RELEASE_FILESTR(READER, file);
    return rc;
}

/* remove a file */
int ctru_remove(const char *path)
{
    logf("remove(path=\"%s\")\n", path);

    int rc;

    file_internal_lock_WRITER();
    Result res = FSUSER_DeleteFile(sdmcArchive,
                                   fsMakePath(PATH_ASCII, path));
    if (R_FAILED(res))
        FILE_ERROR(ERRNO, -1);

    rc = 0;
file_error:
    file_internal_unlock_WRITER();
    return rc;
}

/* rename a file */
int ctru_rename(const char *old, const char *new)
{
    /* note: move by rename does not work in devkitARM toolchain */
    logf("rename(old=\"%s\",new=\"%s\")\n", old, new);

    int rc;

    /* if 'old' is a directory then 'new' is also required to be one if 'new'
       is to be overwritten */
    bool are_dirs = false;

    file_internal_lock_WRITER();

    if (!strcmp(new, old)) /* case-only is ok */
    {
        logf("No name change (success)\n");
        rc = 0;
        FILE_ERROR(ERRNO, _RC);
    }

    /* open 'old'; it must exist */
    Handle open1rc;
    Result res = FSUSER_OpenFile(&open1rc,
	                         sdmcArchive, 
	                         fsMakePath(PATH_ASCII, old),
	                         FS_OPEN_READ,
	                         0);
    if (R_FAILED(res)) {
        /* not a file, try to open a directory */
        res = FSUSER_OpenDirectory(&open1rc,
                                   sdmcArchive,
                                   fsMakePath(PATH_ASCII, old));
        if (R_FAILED(res)) {
            logf("Failed opening old\n");
            FILE_ERROR(ERRNO, -1);
        }

        are_dirs = true;
    }

    if (are_dirs) {
        /* rename directory */
        FSUSER_RenameDirectory(sdmcArchive,
                               fsMakePath(PATH_ASCII, old),
                               sdmcArchive,
                               fsMakePath(PATH_ASCII, new));
    }
    else {
        /* rename file */
        FSUSER_RenameFile(sdmcArchive,
                          fsMakePath(PATH_ASCII, old),
                          sdmcArchive,
                          fsMakePath(PATH_ASCII, new));
    }

    if (R_FAILED(res)) {
        logf("Rename failed\n");
        FILE_ERROR(ERRNO, -2);
    }

    rc = 0;
file_error:
    /* for now, there is nothing to fail upon closing the old stream */
    if (open1rc > 0) {
        if (are_dirs)
            FSDIR_Close(open1rc);
        else
            FSFILE_Close(open1rc);
    }

    file_internal_unlock_WRITER();
    return rc;
}

/** Extensions **/

/* todo: utime does not work in devkitARM toolchain */
int ctru_modtime(const char *path, time_t modtime)
{
    struct utimbuf times =
    {
        .actime = modtime,
        .modtime = modtime,
    };

    return utime(path, &times);
}

/* get the binary size of a file (in bytes) */
off_t ctru_filesize(int fildes)
{
    struct filestr_desc * const file = GET_FILESTR(READER, fildes);
    if (!file)
        FILE_ERROR_RETURN(ERRNO, -1);

    off_t rc;
    file_size_t size = *file->sizep;

    if (size > FILE_SIZE_MAX)
        FILE_ERROR(EOVERFLOW, -2);

    rc = (off_t)size;
file_error:
    RELEASE_FILESTR(READER, file);
    return rc;
}

/* test if two file descriptors refer to the same file */
int ctru_fsamefile(int fildes1, int fildes2)
{
    struct filestr_desc * const file1 = GET_FILESTR(WRITER, fildes1);
    if (!file1)
        FILE_ERROR_RETURN(ERRNO, -1);

    int rc = -2;

    struct filestr_desc * const file2 = get_filestr(fildes2);
    if (file2)
        rc = strcmp(file1->stream.path, file2->stream.path) == 0 ? 1 : 0;

    RELEASE_FILESTR(WRITER, file1);
    return rc;
}

/* tell the relationship of path1 to path2 */
int ctru_relate(const char *path1, const char *path2)
{
    /* FAT32 file system does not support symbolic links,
       therefore, comparing the two full paths should be enough
       to tell relationship */
    logf("relate(path1=\"%s\",path2=\"%s\")\n", path1, path2);
    int rc = RELATE_DIFFERENT;
    if (strcmp(path1, path2) == 0)
        rc = RELATE_SAME;
    return rc;
}

/* test file or directory existence */
bool ctru_file_exists(const char *path)
{
    file_internal_lock_WRITER();
    bool rc = test_stream_exists_internal(path) > 0;
    file_internal_unlock_WRITER();
    return rc;
}

/* note: no symbolic links support in devkitARM */
ssize_t ctru_readlink(const char *path, char *buf, size_t bufsiz)
{
    return readlink(path, buf, bufsiz);
}

