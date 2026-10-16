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

/* check for file existence */
int test_stream_exists_internal(const char *path)
{
    int rc;
    bool is_dir = false;

    char realpath[1024];
    const char *fpath = handle_special_dirs(path, 0x0, realpath,
                                            sizeof (realpath));
    if (!fpath)
        FILE_ERROR_RETURN(ENAMETOOLONG, -1);

    SceUID handle = sceIoOpen(fpath, PSP_O_RDONLY, 0777);
    if (handle <= 0) {
        /* not a file, try to open a directory */
        handle = sceIoDopen(fpath);
        if (handle < 0) {
            logf("File does not exist\n");
            FILE_ERROR(ERRNO, -1);
        }

        is_dir = true;
    }

    rc = 1;
file_error:
    if (handle >= 0) {
        if (is_dir)
            sceIoDclose(handle);
        else
            sceIoClose(handle);
    }

    return rc;
}

int open_internal_inner2(sys_handle_type *handle, const char *path, int flags)
{
    logf("open_internal_inner2(%s, 0x%x)\n", path, flags);
    int rc;
    SceUID fd = sceIoOpen(path, flags, 0777);
    if (fd <= 0) {
        logf("open_internal_inner2: sceIoOpen error (0x%x).\n", fd);
        FILE_ERROR(ERRNO, -1);
    }

    *handle = fd;

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

    int openFlags = PSP_O_RDONLY;
    if (oflag & O_ACCMODE)
    {
        if ((oflag & O_ACCMODE) == O_WRONLY) {
            openFlags = PSP_O_WRONLY;
        }
        if ((oflag & O_ACCMODE) == O_RDWR) {
            openFlags = PSP_O_RDWR;
        }
    }
    else if (oflag & O_TRUNC)
    {
        /* O_TRUNC requires write mode */
        logf("No write mode but have O_TRUNC\n");
        FILE_ERROR(EINVAL, -2);
    }

    /* set truncate flag if requested */
    if (oflag & O_TRUNC) {
        openFlags |= PSP_O_TRUNC;
    }

    /* additional flags */
    if (oflag & O_EXCL) {
        openFlags |= PSP_O_EXCL;
    }
    if (oflag & O_NONBLOCK) {
        openFlags |= PSP_O_NBLOCK;
    }

    /* O_CREAT and O_APPEND are fine without write mode
     * for the former, an empty file is created but no data may be written
     * for the latter, no append will be allowed anyway */
    if (!(oflag & O_CREAT))
        oflag &= ~O_EXCL; /* result is undefined: we choose "ignore" */

    char realpath[1024];
    const char *fpath = handle_special_dirs(path, 0x0, realpath,
                                            sizeof (realpath));

    filestr_base_init(&file->stream);
    rc = open_internal_inner2(&file->stream.handle, fpath, openFlags);

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
        openFlags |= PSP_O_CREAT;
        rc = open_internal_inner2(&file->stream.handle, fpath, openFlags);
        if (rc < 0)
            FILE_ERROR(ERRNO, rc * 10 - 6);
    }
    else
    {
        logf("File not found\n");
        FILE_ERROR(ENOENT, -5);
    }

     /* we will use real file path to implement psp_fsamefile function */
    strcpy(file->stream.path, fpath);

    /* we need to set file size here, or else lseek
       will fail if no read or write has been done */
    SceIoStat st;
    memset(&st, 0, sizeof(SceIoStat));
    if (sceIoGetstat(file->stream.path, &st) < 0) {
        FILE_ERROR(ERRNO, -8);
    }

    file->stream.cache = NewPageReader(file->stream.handle, defaultPageSize);
    if (file->stream.cache == NULL) {
        FILE_ERROR(ERRNO, -7);
    }

    file->stream.flags = oflag;
    file->stream.size = st.st_size;
    file->sizep = &file->stream.size;
    AtomicSet(&file->offset, 0);

    return fildes;

file_error:
    if (fildes >= 0) {
        if (file->stream.cache != NULL) {
            PageReader_Free(file->stream.cache);
            file->stream.cache = NULL;
        }

        sceIoClose(file->stream.handle);
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
        PageReader_Free(file->stream.cache);
        file->stream.cache = NULL;
    }

    sceIoClose(file->stream.handle);
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

    /* hack: truncate file using full path */
    int result = truncate(file->stream.path, length);
    if (result < 0) {
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
    return sceIoSync("ms0:", 0);
    (void) fildes;
}

/* remove a file */
int console_remove(const char *path)
{
    logf("remove(path=\"%s\")\n", path);
    int rc;

    char realpath[1024];
    const char *fpath = handle_special_dirs(path, 0x0, realpath,
                                            sizeof (realpath));

    file_internal_lock_WRITER();
    int result = sceIoRemove(fpath);
    if (result < 0)
        FILE_ERROR(ERRNO, -1);

    rc = 0;
file_error:
    file_internal_unlock_WRITER();
    return rc;
}

/* rename a file */
int console_rename(const char *old, const char *new)
{
    /* note: move by rename does not work in pspsdk */
    logf("rename(old=\"%s\",new=\"%s\")\n", old, new);
    int rc;

    file_internal_lock_WRITER();

    if (!strcmp(new, old)) /* case-only is ok */
    {
        logf("No name change (success)\n");
        rc = 0;
        FILE_ERROR(ERRNO, _RC);
    }

    char realpath[1024];
    const char *fpath_new = handle_special_dirs(new, 0x0, realpath,
                                            sizeof (realpath));

    /* In POSIX-like systems if "new" is the name of an existing file,
       it is silently removed */
    SceIoStat st;
    memset(&st, 0, sizeof(SceIoStat));
    if (sceIoGetstat(fpath_new, &st) >= 0) {
        if (st.st_attr & FIO_SO_IFREG)
            sceIoRemove(fpath_new);
    }

    char realpath_old[1024];
    const char *fpath = handle_special_dirs(old, 0x0, realpath_old,
                                            sizeof (realpath_old));


    int result = sceIoRename(fpath, fpath_new);
    if (result < 0) {
        logf("Rename failed (0x%x)\n", result);
        FILE_ERROR(ERRNO, -2);
    }

    rc = 0;
file_error:
    file_internal_unlock_WRITER();
    return rc;
}

/** Extensions **/

/* todo: utime does not work in devkitARM toolchain */
int console_modtime(const char *path, time_t modtime)
{
    struct tm *info = localtime(&modtime);

    // convert to psp time format
    ScePspDateTime t = {
        .year = info->tm_year + 1900,
        .month = info->tm_mon + 1,
        .hour = info->tm_hour,
        .minute = info->tm_min,
        .second = info->tm_sec,
        .microsecond = 0
    };

    SceIoStat st;
    st.sce_st_atime = t;
    st.sce_st_mtime = t;

    char realpath[1024];
    const char *fpath = handle_special_dirs(path, 0x0, realpath,
                                            sizeof (realpath));

    int result = sceIoChstat(fpath, &st, 0xff);
    if (result < 0) {
        logf("psp_modtime: sceIoChstat failed (0x%x)\n", result);
        return -1;
    }

    return 0;
}

/* note: no symbolic links support in devkitARM */
ssize_t console_readlink(const char *path, char *buf, size_t bufsiz)
{
    logf("readlink(path=%s)\n", path);
    /* not implemented */
    return -1;
    (void) path;
    (void) buf;
    (void) bufsiz;
}
