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
#define DIRFUNCTIONS_DEFINED
#include "config.h"
#include <errno.h>
#include <string.h>
#include "debug.h"
#include "dir.h"
#include "pathfuncs.h"
#include "fileobj_mgr.h"
#include "dircache_redirect.h"

/* structure used for open directory streams */
static struct dirstr_desc
{
    struct filestr_base stream; /* basic stream info (first!) */
    struct dirscan_info scan;   /* directory scan cursor */
    struct dirent       entry;  /* current parsed entry information */
#ifdef HAVE_MULTIVOLUME
    int                 volumecounter; /* counter for root volume entries */
#endif
} open_streams[MAX_OPEN_DIRS];

/* check and return a struct dirstr_desc* from a DIR* */
static struct dirstr_desc * get_dirstr(DIR *dirp)
{
    struct dirstr_desc *dir = (struct dirstr_desc *)dirp;

    if (!PTR_IN_ARRAY(open_streams, dir, MAX_OPEN_DIRS))
        dir = NULL;
    else if (dir->stream.flags & FDO_BUSY)
        return dir;

    int errnum;

    if (!dir)
    {
        errnum = EFAULT;
    }
    else if (dir->stream.flags & FD_NONEXIST)
    {
        DEBUGF("dir #%d: nonexistant device\n", (int)(dir - open_streams));
        errnum = ENXIO;
    }
    else
    {
        DEBUGF("dir #%d: dir not open\n", (int)(dir - open_streams));
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
        else                                         \
            file_internal_unlock_##type();           \
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
        struct dirstr_desc *dir = &open_streams[dd];
        if (!dir->stream.flags)
            return dir;
    }

    DEBUGF("Too many dirs open\n");
    return NULL;
}

#ifdef HAVE_MULTIVOLUME
static int readdir_volume_inner(struct dirstr_desc *dir, struct dirent *entry)
{
    /* Volumes (secondary file systems) get inserted into the system root
     * directory. If the path specified volume 0, enumeration will not
     * include other volumes, but just its own files and directories.
     *
     * Fake special directories, which don't really exist, that will get
     * redirected upon opendir()
     */
    while (++dir->volumecounter < NUM_VOLUMES)
    {
        /* on the system root */
        if (!fat_ismounted(dir->volumecounter))
            continue;

        get_volume_name(dir->volumecounter, entry->d_name);
        dir->entry.info.attr    = ATTR_MOUNT_POINT;
        dir->entry.info.size    = 0;
        dir->entry.info.wrtdate = 0;
        dir->entry.info.wrttime = 0;
        return 1;
    }

    /* do normal directory entry fetching */
    return 0;
}
#endif /* HAVE_MULTIVOLUME */

static inline int readdir_volume(struct dirstr_desc *dir,
                                 struct dirent *entry)
{
#ifdef HAVE_MULTIVOLUME
    /* fetch virtual volume entries? */
    if (dir->volumecounter < NUM_VOLUMES)
        return readdir_volume_inner(dir, entry);
#endif /* HAVE_MULTIVOLUME */

    /* do normal directory entry fetching */
    return 0;
    (void)dir; (void)entry;
}


/** POSIX interface **/

/* open a directory */
DIR * opendir(const char *dirname)
{
    DEBUGF("opendir(dirname=\"%s\"\n", dirname);

    DIR *dirp = NULL;

    file_internal_lock_WRITER();

    int rc;

    struct dirstr_desc * const dir = alloc_dirstr();
    if (!dir)
        FILE_ERROR(EMFILE, RC);

    rc = open_stream_internal(dirname, FF_DIR, &dir->stream, NULL);
    if (rc < 0)
    {
        DEBUGF("Open failed: %d\n", rc);
        FILE_ERROR(ERRNO, RC);
    }

#ifdef HAVE_MULTIVOLUME
    /* volume counter is relevant only to the system root */
    dir->volumecounter = rc > 1 ? 0 : INT_MAX;
#endif /* HAVE_MULTIVOLUME */

    fat_rewind(&dir->stream.fatstr);
    rewinddir_dirent(&dir->scan);

    dirp = (DIR *)dir;
file_error:
    file_internal_unlock_WRITER();
    return dirp;
}

/* close a directory stream */
int closedir(DIR *dirp)
{
    int rc;

    file_internal_lock_WRITER();

    /* needs to work even if marked "nonexistant" */
    struct dirstr_desc * const dir = (struct dirstr_desc *)dirp;
    if (!PTR_IN_ARRAY(open_streams, dir, MAX_OPEN_DIRS))
        FILE_ERROR(EFAULT, -1);

    if (!dir->stream.flags)
    {
        DEBUGF("dir #%d: dir not open\n", (int)(dir - open_streams));
        FILE_ERROR(EBADF, -2);
    }

    rc = close_stream_internal(&dir->stream);
    if (rc < 0)
        FILE_ERROR(ERRNO, rc * 10 - 3);

file_error:
    file_internal_unlock_WRITER();
    return rc;
}

/* read a directory */
struct dirent * readdir(DIR *dirp)
{
    struct dirstr_desc * const dir = GET_DIRSTR(READER, dirp);
    if (!dir)
        FILE_ERROR_RETURN(ERRNO, NULL);

    struct dirent *res = NULL;

    int rc = readdir_volume(dir, &dir->entry);
    if (rc == 0)
    {
        rc = readdir_dirent(&dir->stream, &dir->scan, &dir->entry);
        if (rc < 0)
            FILE_ERROR(EIO, RC);
    }

    if (rc > 0)
        res = &dir->entry;

file_error:
    RELEASE_DIRSTR(READER, dir);

    if (rc > 1)
        iso_decode_d_name(res->d_name);

    return res;
}

#if 0 /* not included now but probably should be */
/* read a directory (reentrant) */
int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result)
{
    if (!result)
        FILE_ERROR_RETURN(EFAULT, -2);

    *result = NULL;

    if (!entry)
        FILE_ERROR_RETURN(EFAULT, -3);

    struct dirstr_desc * const dir = GET_DIRSTR(READER, dirp);
    if (!dir)
        FILE_ERROR_RETURN(ERRNO, -1);

    int rc = readdir_volume(dir, entry);
    if (rc == 0)
    {
        rc = readdir_dirent(&dir->stream, &dir->scan, entry);
        if (rc < 0)
            FILE_ERROR(EIO, rc * 10 - 4);
    }

file_error:
    RELEASE_DIRSTR(READER, dir);

    if (rc > 0)
    {
        if (rc > 1)
            iso_decode_d_name(entry->d_name);

        *result = entry;
        rc = 0;
    }

    return rc;
}

/* reset the position of a directory stream to the beginning of a directory */
void rewinddir(DIR *dirp)
{
    struct dirstr_desc * const dir = GET_DIRSTR(READER, dirp);
    if (!dir)
        FILE_ERROR_RETURN(ERRNO);

    rewinddir_dirent(&dir->scan);

#ifdef HAVE_MULTIVOLUME
    if (dir->volumecounter != INT_MAX)
        dir->volumecounter = 0;
#endif /* HAVE_MULTIVOLUME */

    RELEASE_DIRSTR(READER, dir);
}

#endif /* 0 */

/* make a directory */
int mkdir(const char *path)
{
    DEBUGF("mkdir(path=\"%s\")\n", path);

    int rc;

    file_internal_lock_WRITER();

    struct filestr_base stream;
    struct path_component_info compinfo;
    rc = open_stream_internal(path, FF_DIR | FF_PARENTINFO, &stream,
                              &compinfo);
    if (rc < 0)
    {
        DEBUGF("Can't open parent dir or path is not a directory\n");
        FILE_ERROR(ERRNO, rc * 10 - 1);
    }
    else if (rc > 0)
    {
        DEBUGF("File exists\n");
        FILE_ERROR(EEXIST, -2);
    }

    rc = create_stream_internal(&compinfo.parentinfo, compinfo.name,
                                compinfo.length, ATTR_NEW_DIRECTORY,
                                FO_DIRECTORY, &stream);
    if (rc < 0)
        FILE_ERROR(ERRNO, rc * 10 - 3);

    rc = 0;
file_error:
    close_stream_internal(&stream);
    file_internal_unlock_WRITER();
    return rc;
}

/* remove a directory */
int rmdir(const char *name)
{
    DEBUGF("rmdir(name=\"%s\")\n", name);

    if (name)
    {
        /* path may not end with "." */
        const char *basename;
        size_t len = path_basename(name, &basename);
        if (basename[0] == '.' && len == 1)
        {
            DEBUGF("Invalid path; last component is \".\"\n");
            FILE_ERROR_RETURN(EINVAL, -9);
        }
    }

    file_internal_lock_WRITER();
    int rc = remove_stream_internal(name, NULL, FF_DIR);
    file_internal_unlock_WRITER();
    return rc;
}


/** Extended interface **/

/* return if two directory streams refer to the same directory */
int samedir(DIR *dirp1, DIR *dirp2)
{
    struct dirstr_desc * const dir1 = GET_DIRSTR(WRITER, dirp1);
    if (!dir1)
        FILE_ERROR_RETURN(ERRNO, -1);

    int rc = -2;

    struct dirstr_desc * const dir2 = get_dirstr(dirp2);
    if (dir2)
        rc = dir1->stream.bindp == dir2->stream.bindp ? 1 : 0;

    RELEASE_DIRSTR(WRITER, dir1);
    return rc;
}

/* test directory existence (returns 'false' if a file) */
bool dir_exists(const char *dirname)
{
    file_internal_lock_WRITER();
    bool rc = test_stream_exists_internal(dirname, FF_DIR) > 0;
    file_internal_unlock_WRITER();
    return rc;
}

/* get the portable info from the native entry */
struct dirinfo dir_get_info(DIR *dirp, struct dirent *entry)
{
    int rc;
    if (!dirp || !entry)
        FILE_ERROR(EFAULT, RC);

    if (entry->d_name[0] == '\0')
        FILE_ERROR(ENOENT, RC);

    if ((file_size_t)entry->info.size > FILE_SIZE_MAX)
        FILE_ERROR(EOVERFLOW, RC);

    return (struct dirinfo)
    {
        .attribute = entry->info.attr,
        .size      = entry->info.size,
        .mtime     = fattime_mktime(entry->info.wrtdate, entry->info.wrttime),
    };

file_error:
    return (struct dirinfo){ .attribute = 0 };
}
