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
#define DIRFUNCTIONS_DEFINED
#include "config.h"
#include <errno.h>
#include <string.h>
#include "debug.h"
#include "dir.h"
#include "pathfuncs.h"
#include "timefuncs.h"
#include "system.h"
#include "fs_defines.h"
#include "rbunicode.h"

#include "sys_file.h"
#include "sys_file_internal.h"

/* This file is based on firmware/common/dir.c */

/* Define LOGF_ENABLE to enable logf output in this file */
// #define LOGF_ENABLE
#include "logf.h"

/* structure used for open directory streams */
static struct dirstr_desc
{
    struct filestr_base stream; /* basic stream info (first!) */
    struct dirent       entry;  /* current parsed entry information */
} open_dir_streams[MAX_OPEN_DIRS] =
{
    [0 ... MAX_OPEN_DIRS-1] = { .stream = { .handle = -1 } }
};

/* check and return a struct dirstr_desc* from a DIR* */
static struct dirstr_desc * get_dirstr(DIR *dirp)
{
    struct dirstr_desc *dir = (struct dirstr_desc *)dirp;

    if (!PTR_IN_ARRAY(open_dir_streams, dir, MAX_OPEN_DIRS))
        dir = NULL;
    else if (dir->stream.handle >= 0)
        return dir;

    int errnum;

    if (!dir)
    {
        errnum = EFAULT;
    }
    else
    {
        logf("dir #%d: dir not open\n", (int)(dir - open_dir_streams));
        errnum = EBADF;
    }

    errno = errnum;
    return NULL;
}

#define GET_DIRSTR(type, dirp) \
    ({                                               \
        file_internal_lock_##type();                 \
        struct dirstr_desc *_dir = get_dirstr(dirp); \
        if (_dir)                                    \
            FILESTR_LOCK(type, &_dir->stream);       \
        else     {                                    \
            file_internal_unlock_##type();           \
} \
        _dir;                                        \
    })

/* release the lock on the dirstr_desc* */
#define RELEASE_DIRSTR(type, dir) \
    ({                                        \
        FILESTR_UNLOCK(type, &(dir)->stream); \
        file_internal_unlock_##type();        \
    })


/* find a free dir stream descriptor */
static struct dirstr_desc * alloc_dirstr(void)
{
    for (unsigned int dd = 0; dd < MAX_OPEN_DIRS; dd++)
    {
        struct dirstr_desc *dir = &open_dir_streams[dd];
        if (dir->stream.handle == -1)
            return dir;
    }

    logf("Too many dirs open\n");
    return NULL;
}

/** POSIX interface **/

/* open a directory */
DIR * console_opendir(const char *dirname)
{
    logf("opendir(dirname=\"%s\")\n", dirname);

    DIR *dirp = NULL;
    file_internal_lock_WRITER();

    int rc;

    struct dirstr_desc * const dir = alloc_dirstr();
    if (!dir)
        FILE_ERROR(EMFILE, _RC);

    char realpath[1024];
    const char *fpath = handle_special_dirs(dirname, 0x0, realpath,
                                            sizeof (realpath));

    filestr_base_init(&dir->stream);
    dir->stream.handle = sceIoDopen(fpath);
    if (dir->stream.handle < 0) {
        logf("sceIoDopen failed: 0x%x\n", dir->stream.handle);
        FILE_ERROR(EMFILE, -1);
    }

    dir->stream.size = 0;
    dir->stream.flags = 0;

    /* we will use file path to implement console_samedir function */
    strcpy(dir->stream.path, dirname);

    dirp = (DIR *)dir;
file_error:
    file_internal_unlock_WRITER();
    return dirp;
}

/* close a directory stream */
int console_closedir(DIR *dirp)
{
    int rc;

    file_internal_lock_WRITER();

    /* needs to work even if marked "nonexistant" */
    struct dirstr_desc * const dir = (struct dirstr_desc *)dirp;
    if (!PTR_IN_ARRAY(open_dir_streams, dir, MAX_OPEN_DIRS))
        FILE_ERROR(EFAULT, -1);

    logf("closedir(dirname=\"%s\")\n", dir->stream.path);

    if (dir->stream.handle == 0)
    {
        logf("dir #%d: dir not open\n", (int)(dir - open_dir_streams));
        FILE_ERROR(EBADF, -2);
    }

    int result = sceIoDclose(dir->stream.handle);
    if (result < 0)
        FILE_ERROR(ERRNO, -3);

    dir->stream.handle = -1;
    dir->stream.path[0] = '\0';

    rc = 0;
file_error:
    file_internal_unlock_WRITER();
    return rc;
}

void dirstr_entry_init(SceIoDirent *dirEntry, struct dirent *entry)
{
    /* clear */
    memset(entry, 0, sizeof(struct dirent));

    SceIoStat st = dirEntry->d_stat;

    /* attributes */
    if (st.st_attr & FIO_SO_IFDIR)
        entry->info.attr |= ATTR_DIRECTORY;
    if (st.st_attr & FIO_SO_IFREG)
        entry->info.attr |= ATTR_ARCHIVE;
    if (dirEntry->d_name[0] == '.')
        entry->info.attr |= ATTR_HIDDEN;

    /* size */
    entry->info.size = st.st_size;

    /* name */
    SceIoFatDirentPrivate *fatDirEntry = dirEntry->d_private;
    /* uint8_t d_name[0x400 + 1];
    memset(d_name, '\0', 0x400);
    utf16decode(fatDirEntry->l_name, d_name, 1024, sizeof(d_name), true); */
    memcpy(entry->d_name, fatDirEntry->l_name, 0x400);
    /* DEBUGF("entry: %s\n", entry->d_name); */
}

/* read a directory */
struct dirent * console_readdir(DIR *dirp)
{
    struct dirstr_desc * const dir = GET_DIRSTR(READER, dirp);
    if (!dir)
        FILE_ERROR_RETURN(ERRNO, NULL);

    int rc;
    struct dirent *res = NULL;

    logf("readdir(dirname=\"%s\")\n", dir->stream.path);

    SceIoDirent entry;
        SceIoFatDirentPrivate fat_entry;
        memset(&entry, 0, sizeof(SceIoDirent));
        memset(&fat_entry, 0, sizeof(SceIoFatDirentPrivate));
        fat_entry.size = sizeof(SceIoFatDirentPrivate);
        entry.d_private = &fat_entry;
    int result = sceIoDread(dir->stream.handle, &entry);
    if (result < 0)
        FILE_ERROR(EIO, _RC);

    if (result == 0) {
        /* directory end. return NULL value, no errno */
        res = NULL;
        goto file_error;
    }

    res = &dir->entry;
    dirstr_entry_init(&entry, res);

    /* time */
    SceIoStat st = entry.d_stat;
    struct ScePspDateTime *mt = &st.sce_st_mtime;

    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));

    tm.tm_sec  = mt->second;
    tm.tm_min  = mt->minute;
    tm.tm_hour = mt->hour;
    tm.tm_mday = mt->day;
    tm.tm_mon  = mt->month - 1;
    tm.tm_year = mt->year - 1900;

    time_t mtime = mktime(&tm);

    /* DEBUGF("archive_getmtime(%s): %lld\n", full_path, mtime); */

    uint16_t dosdate, dostime;
    dostime_localtime(mtime, &dosdate, &dostime);
    res->info.wrtdate = dosdate;
    res->info.wrttime = dostime;

file_error:
    RELEASE_DIRSTR(READER, dir);
    return res;
}

/* make a directory */
int console_mkdir(const char *path)
{
    logf("mkdir(path=\"%s\")\n", path);

    int rc;

    char realpath[1024];
    const char *fpath = handle_special_dirs(path, 0x0, realpath,
                                            sizeof (realpath));

    file_internal_lock_WRITER();
    int result = sceIoMkdir(fpath, 0777);
    if (result < 0)
        FILE_ERROR(ERRNO, -1);

    rc = 0;
file_error:
    file_internal_unlock_WRITER();
    return rc;
}

/* remove a directory */
int console_rmdir(const char *name)
{
    logf("rmdir(name=\"%s\")\n", name);

    int rc;

    if (name)
    {
        /* path may not end with "." */
        const char *basename;
        size_t len = path_basename(name, &basename);
        if (basename[0] == '.' && len == 1)
        {
            logf("Invalid path; last component is \".\"\n");
            FILE_ERROR_RETURN(EINVAL, -9);
        }
    }

    char realpath[1024];
    const char *fpath = handle_special_dirs(name, 0x0, realpath,
                                            sizeof (realpath));

    file_internal_lock_WRITER();
    int result = sceIoRmdir(fpath);
    if (result < 0)
        FILE_ERROR(ERRNO, -1);

    rc = 0;
file_error:
    file_internal_unlock_WRITER();
    return rc;
}


/** Extended interface **/

/* return if two directory streams refer to the same directory */
int console_samedir(DIR *dirp1, DIR *dirp2)
{
    struct dirstr_desc * const dir1 = GET_DIRSTR(WRITER, dirp1);
    if (!dir1)
        FILE_ERROR_RETURN(ERRNO, -1);

    int rc = -2;

    struct dirstr_desc * const dir2 = get_dirstr(dirp2);
    if (dir2) {
        rc = strcmp(dir1->stream.path, dir2->stream.path) == 0 ? 1 : 0;
    }

    RELEASE_DIRSTR(WRITER, dir1);
    return rc;
}

/* test directory existence (returns 'false' if a file) */
bool console_dir_exists(const char *dirname)
{
    file_internal_lock_WRITER();

    char realpath[1024];
    const char *fpath = handle_special_dirs(dirname, 0x0, realpath,
                                            sizeof (realpath));

    int rc;
    SceUID handle = sceIoDopen(fpath);
    if (handle < 0) {
        logf("Directory not found (0x%x)\n", handle);
        FILE_ERROR(EMFILE, -1);
    }

    rc = 0;
file_error:
    if (rc == 0) {
        sceIoDclose(handle);
    }
    file_internal_unlock_WRITER();
    return rc == 0 ? true : false;
}

/* get the portable info from the native entry */
struct dirinfo console_dir_get_info(DIR *dirp, struct dirent *entry)
{
    int rc;
    if (!dirp || !entry)
        FILE_ERROR(EFAULT, _RC);

    if (entry->d_name[0] == '\0')
        FILE_ERROR(ENOENT, _RC);

    if ((file_size_t)entry->info.size > FILE_SIZE_MAX)
        FILE_ERROR(EOVERFLOW, _RC);

    return (struct dirinfo)
    {
        .attribute = entry->info.attr,
        .size      = entry->info.size,
        .mtime     = dostime_mktime(entry->info.wrtdate, entry->info.wrttime),
    };

file_error:
    return (struct dirinfo){ .attribute = 0 };
}

const char* console_root_realpath(void)
{
    /* Native only, for APP and SIM see respective filesystem-.c files */
    return PATH_ROOTSTR; /* rb_namespace.c */
}