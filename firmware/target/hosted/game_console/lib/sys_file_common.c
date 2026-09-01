/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include "fs_defines.h"

/* this includes a couple of 3ds headers */
#include "sys_file.h"
#include "sys_file_internal.h"

#define RB_FILESYSTEM_OS
#include "config.h"
#include "system.h"
#include "file.h"
#include "debug.h"
#include "string-extra.h"

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

struct filestr_desc open_streams[MAX_OPEN_FILES] =
{
    [0 ... MAX_OPEN_FILES-1] = { .stream = { .cache = NULL, .flags = 0 } }
};

/* check and return a struct filestr_desc* from a file descriptor number */
struct filestr_desc * get_filestr(int fildes)
{
    struct filestr_desc *file = &open_streams[fildes];

    if ((unsigned int)fildes >= MAX_OPEN_FILES)
        file = NULL;
    else if (file->stream.cache != NULL)
        return file;

    logf("fildes %d: bad file number\n", fildes);
    errno = (file && (file->stream.cache == NULL)) ? ENXIO : EBADF;
    return NULL;
}

/* find a free file descriptor */
int alloc_filestr(struct filestr_desc **filep)
{
    for (int fildes = 0; fildes < MAX_OPEN_FILES; fildes++)
    {
        struct filestr_desc *file = &open_streams[fildes];
        if (file->stream.cache == NULL)
        {
            *filep = file;
            return fildes;
        }
    }

    logf("Too many files open\n");
    return -1;
}

/* set the file pointer */
off_t lseek_internal(struct filestr_desc *file, off_t offset,
                            int whence)
{
    off_t rc;
    off_t pos;

    off_t size = MIN(*file->sizep, FILE_SIZE_MAX);
    off_t file_offset = AtomicGet(&file->offset);

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

    AtomicSet(&file->offset, pos);

    return pos;
file_error:
    return rc;
}

/* read from or write to the file; back end to read() and write() */
ssize_t readwrite(struct filestr_desc *file, void *buf, size_t nbyte,
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
            AtomicSet(&file->offset, MIN(size, FILE_SIZE_MAX));

        filerem = FILE_SIZE_MAX - AtomicGet(&file->offset);
    }
    else
    {
        /* limit to maximum possible offset (EOF or FILE_SIZE_MAX) */
        filerem = MIN(size, FILE_SIZE_MAX) - AtomicGet(&file->offset);
    }

    if (nbyte > filerem)
    {
        nbyte = filerem;
        if (nbyte > 0)
            {}
        else if (write)
            FILE_ERROR_RETURN(EFBIG, -1);     /* would get too large */
        else if (AtomicGet(&file->offset) >= FILE_SIZE_MAX)
            FILE_ERROR_RETURN(EOVERFLOW, -2); /* can't read here */
    }

    if (nbyte == 0)
        return 0;

    int rc = 0;
    int_error_t n_err;

    if (write) {
        n_err = PageReader_WriteAt(file->stream.cache,
                                   buf,
                                   nbyte,
                                   AtomicGet(&file->offset),
                                   file->stream.flags & O_RDWR ? true : false);
    }
    else {
        n_err = PageReader_ReadAt(file->stream.cache,
                                  buf,
                                  nbyte,
                                  AtomicGet(&file->offset));
    }

    if ((n_err.err != NULL) && strcmp(n_err.err, "io.EOF")) {
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
        if (write && AtomicGet(&file->offset) > size)
            *file->sizep = AtomicGet(&file->offset);

        return done;
    }

    return rc;
}

/* initialize the base descriptor */
void filestr_base_init(struct filestr_base *stream)
{
    stream->cache = NULL;
    stream->handle = 0;
    stream->size = 0;
    sys_mutex_init(&stream->mtx);
}

int open_internal_inner1(const char *path, int oflag);
int open_internal_locked(const char *path, int oflag)
{
    file_internal_lock_WRITER();
    int rc = open_internal_inner1(path, oflag);
    file_internal_unlock_WRITER();
    return rc;
}

int console_open(const char *path, int oflag, ...)
{
    logf("open(path=\"%s\",oflag=%X)\n", path, (unsigned)oflag);

    /* we need to wait a little here when playing a folder with many small files
       to prevent other threads starvation */
    sleep(10);

    return open_internal_locked(path, oflag);
}

int console_creat(const char *path, mode_t mode)
{
    logf("creat(path=\"%s\")\n", path);
    return console_open(path, O_WRONLY|O_CREAT|O_TRUNC, mode);
}

/* move the read/write file offset */
off_t console_lseek(int fildes, off_t offset, int whence)
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
ssize_t console_read(int fildes, void *buf, size_t nbyte)
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
ssize_t console_write(int fildes, const void *buf, size_t nbyte)
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

/** Extensions **/

/* get the binary size of a file (in bytes) */
off_t console_filesize(int fildes)
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
int console_fsamefile(int fildes1, int fildes2)
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
int console_relate(const char *path1, const char *path2)
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
int test_stream_exists_internal(const char *path);
bool console_file_exists(const char *path)
{
    file_internal_lock_WRITER();
    bool rc = test_stream_exists_internal(path) > 0;
    file_internal_unlock_WRITER();
    return rc;
}