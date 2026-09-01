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

extern FS_Archive sdmcArchive;

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

int open_internal_inner1(const char *path, int oflag)
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

    file->stream.cache = NewPageReader(file->stream.handle, defaultPageSize);
    if (file->stream.cache == NULL) {
        FILE_ERROR(ERRNO, -7);
    }

    file->stream.flags = oflag;
    file->stream.size = size;
    file->sizep = &file->stream.size;
    AtomicSet(&file->offset, 0);

    /* we will use file path to implement console_fsamefile function */
    strcpy(file->stream.path, path);

    return fildes;

file_error:
    if (fildes >= 0) {
        if (file->stream.cache != NULL) {
            /* FSFILE_Flush(file->stream.handle); */
            PageReader_Free(file->stream.cache);
            file->stream.cache = NULL;
        }
    
        FSFILE_Close(file->stream.handle);
        file->stream.handle = 0;
    }

    return rc;
}

int console_close(int fildes)
{
    logf("close(fd=%d)\n", fildes);

    int rc;

    file_internal_lock_WRITER();

    /* needs to work even if marked "nonexistant" */
    struct filestr_desc *file = &open_streams[fildes];
    if ((unsigned int)fildes >= MAX_OPEN_FILES || (file->stream.cache == NULL))
    {
        logf("filedes %d not open\n", fildes);
        FILE_ERROR(EBADF, -2);
    }

    if (file->stream.cache != NULL) {
        /* FSFILE_Flush(file->stream.handle); */
        PageReader_Free(file->stream.cache);
        file->stream.cache = NULL;
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
int console_ftruncate(int fildes, off_t length)
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

    Result res = FSFILE_SetSize(file->stream.handle, length);
    if (R_FAILED(res)) {
        FILE_ERROR(ERRNO, -11);
    }

    *file->sizep = length;

    rc = 0;
file_error:
    RELEASE_FILESTR(READER, file);
    return rc;
}

/* synchronize changes to a file */
int console_fsync(int fildes)
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
    Result res = FSFILE_Flush(file->stream.handle);
    if (R_FAILED(res)) {
        FILE_ERROR(ERRNO, -3);
    }

    rc = 0;
file_error:
    RELEASE_FILESTR(WRITER, file);
    return rc;
}

/* remove a file */
int console_remove(const char *path)
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
int console_rename(const char *old, const char *new)
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
int console_modtime(const char *path, time_t modtime)
{
    struct utimbuf times =
    {
        .actime = modtime,
        .modtime = modtime,
    };

    return utime(path, &times);
}

/* note: no symbolic links support in devkitARM */
ssize_t console_readlink(const char *path, char *buf, size_t bufsiz)
{
    return readlink(path, buf, bufsiz);
}
