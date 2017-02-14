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
#include "pathfuncs.h"
#include "file_internal.h"
#include "fileobj_mgr.h"
#include "dircache_redirect.h"
#include "dir.h"

/* structure used for open directory streams */
static struct dirstr_desc
{
    struct filestr *strp; /* basic stream info (first!) */
    struct dirscan scan;  /* directory scan cursor */
    struct dirent  entry; /* current parsed entry information */
#ifdef HAVE_MULTIVOLUME
    int    volumecounter; /* counter for root volume entries */
#endif
} open_dirs[MAX_OPEN_DIRS];

/* check and return a struct dirstr_desc* from a DIR* */
static struct dirstr_desc * get_dirstr(DIR *dirp)
{
    struct dirstr_desc *dir = (struct dirstr_desc *)dirp;

    if (!PTR_IN_ARRAY(open_dirs, dir, MAX_OPEN_DIRS))
        dir = NULL;
    else if (LIKELY(dir->strp && (dir->strp->flags & FDO_BUSY)))
        return dir;

    int errnum;

    if (!dir)
    {
        errnum = EFAULT;
    }
    else if (dir->strp->flags & FD_NONEXIST)
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
            FILESTR_LOCK(type, _dir->strp);          \
        else                                         \
            file_internal_unlock_##type();           \
        _dir;                                        \
    })

/* release the lock on the dirstr_desc* */
#define RELEASE_DIRSTR(type, dir) \
    ({                                     \
        FILESTR_UNLOCK(type, (dir)->strp); \
        file_internal_unlock_##type();     \
    })


/* find a free dir stream descriptor */
static struct dirstr_desc * alloc_dirstr(void)
{
    for (unsigned int dd = 0; dd < MAX_OPEN_DIRS; dd++)
    {
        struct dirstr_desc *dir = &open_dirs[dd];
        if (!dir->strp)
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
        if (!volume_ismounted_internal(dir->volumecounter))
            continue;

        get_volume_name(dir->volumecounter, entry->d_name);
        dir->entry.info.attr    = ATTR_MOUNT_POINT;
        dir->entry.info.size    = 0;
        dir->entry.info.wrtdate = 0;
        dir->entry.info.wrttime = 0;
        return FSI_S_RDDIR_OK;
    }

    /* do normal directory entry fetching */
    return FSI_S_RDDIR_EOD;
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
    return FSI_S_RDDIR_EOD;
    (void)dir; (void)entry;
}


/** POSIX interface **/

/* open a directory */
DIR * opendir(const char *dirname)
{
    DEBUGF("%s:dirname='%s'\n", __func__, dirname);

    DIR *dirp = NULL;
    int rc;

    file_internal_lock_WRITER();

    struct dirstr_desc * const dir = alloc_dirstr();
    if (!dir)
        FILE_ERROR_RC(-EMFILE);

    rc = file_fileop_open_filestr(dirname, O_RDONLY, FF_DIR, &dir->strp);
    if (rc < 0)
    {
        /* dir->strp will be null */
        DEBUGF("%s failed:%d\n", __func__, rc);
        FILE_ERROR_RC(RC);
    }

#ifdef HAVE_MULTIVOLUME
    /* volume counter is relevant only to the system root */
    unsigned int flags = dir->strp->infop->flags;
    dir->volumecounter = (flags & FF_SYSROOT) ? 0 : INT_MAX;
#endif /* HAVE_MULTIVOLUME */

    rewinddir_internal(dir->strp, &dir->scan);

    dirp = (DIR *)dir;
file_error:
    file_internal_unlock_WRITER();

    SET_ERRNO_RC(rc);
    return dirp;
}

/* close a directory stream */
int closedir(DIR *dirp)
{
    DEBUGF("%s:dirp=%p\n", __func__, dirp);

    int rc;

    file_internal_lock_WRITER();

    struct dirstr_desc * const dir = (struct dirstr_desc *)dirp;
    if (!PTR_IN_ARRAY(open_dirs, dir, MAX_OPEN_DIRS))
        FILE_ERROR_RC(-EFAULT);

    rc = file_fileop_close_filestr(dir->strp);
    if (rc < 0)
    {
        DEBUGF("%s failed:%d\n", __func__, rc);
        FILE_ERROR_RC(RC);
    }

    dir->strp = NULL;
file_error:
    file_internal_unlock_WRITER();

    SET_ERRNO_RC(rc, -1);
    return rc;
}

/* read a directory */
struct dirent * readdir(DIR *dirp)
{
    struct dirstr_desc * const dir = GET_DIRSTR(READER, dirp);
    if (!dir)
        return NULL;

    int rc;
    struct dirent *res = NULL;

    rc = readdir_volume(dir, &dir->entry);
    if (rc == FSI_S_RDDIR_EOD)
    {
        rc = readdir_internal(dir->strp, &dir->scan, &dir->entry);
        if (rc < 0)
        {
            DEBUGF("%s failed:%d\n", __func__, rc);
            FILE_ERROR_RC(-EIO);
        }
    }

    if (FSI_S_RDDIR_HAVEENT(rc))
        res = &dir->entry;

file_error:
    RELEASE_DIRSTR(READER, dir);

    SET_ERRNO_RC(rc);
    return res;
}

#if 0 /* not included now but probably should be */
/* read a directory (reentrant) */
int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result)
{
    struct dirstr_desc * const dir = GET_DIRSTR(READER, dirp);
    if (!dir)
        return -1;

    int rc;

    if (!result)
        FILE_ERROR_RC_RETURN(-EFAULT, -1);

    *result = NULL;

    if (!entry)
        FILE_ERROR_RC_RETURN(-EFAULT, -1);

    rc = readdir_volume(dir, entry);
    if (rc == FSI_S_RDDIR_EOD)
    {
        rc = readdir_internal(dir->strp, &dir->scan, entry);
        if (rc < 0)
        {
            DEBUGF("%s failed:%d\n", __func__, rc);
            FILE_ERROR_RC(-EIO);
        }
    }

    if (FSI_S_RDDIR_HAVEENT(rc))
        *result = entry;

    rc = 0;
file_error:
    RELEASE_DIRSTR(READER, dir);

    SET_ERRNO_RC(rc, -1);
    return rc;
}

/* reset the position of a directory stream to the beginning of a directory */
void rewinddir(DIR *dirp)
{
    DEBUGF("%s:dirp=%p\n", __func__, dirp);

    struct dirstr_desc * const dir = GET_DIRSTR(READER, dirp);
    if (!dir)
        return;

    rewinddir_internal(dir->strp, &dir->scan);

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
    DEBUGF("%s:path='%s'\n", __func__, path);

    int rc;

    file_internal_lock_WRITER();

    struct path_component_info ci;
    rc = open_file_internal(path, FF_PARENTINFO, &ci, NULL);
    if (rc < 0)
    {
        DEBUGF("Can't open parent dir\n");
        FILE_ERROR_RC(RC);
    }
    else if (FSI_S_OPENED(rc))
    {
        DEBUGF("File exists\n");
        FILE_ERROR_RC(-EEXIST);
    }

    rc = create_file_internal(&ci.parentinfo, ci.name, ci.namelen,
                              FO_DIRECTORY, NULL);
    if (rc < 0)
        FILE_ERROR_RC(RC);

    rc = 0;
file_error:
    file_internal_unlock_WRITER();

    SET_ERRNO_RC(rc, -1);
    return rc;
}

/* remove a directory */
int rmdir(const char *name)
{
    DEBUGF("%s:name='%s'\n", __func__, name);

    if (name)
    {
        /* path may not end with "." */
        const char *basename;
        size_t len = path_basename(name, &basename);
        if (basename[0] == '.' && len == 1)
        {
            DEBUGF("Invalid path; last component is '.'\n");
            SET_ERRNO_RC_RETURN(-EINVAL, -1);
        }
    }

    int rc;

    file_internal_lock_WRITER();

    rc = remove_file_internal(name, FF_DIR, NULL);
    if (rc < 0)
       FILE_ERROR_RC(RC);

    rc = 0;
file_error:
    file_internal_unlock_WRITER();

    SET_ERRNO_RC(rc, -1);
    return rc;
}


/** Extended interface **/

/* return if two directory streams refer to the same directory */
int samedir(DIR *dirp1, DIR *dirp2)
{
    DEBUGF("%s:dirp1=%p:dirp2=%p\n", __func__, dirp1, dirp2);
    int rc = -1;

    struct dirstr_desc * const dir1 = GET_DIRSTR(WRITER, dirp1);
    if (dir1)
    {
        struct dirstr_desc * const dir2 = get_dirstr(dirp2);
        if (dir2)
            rc = dir1->strp->infop == dir2->strp->infop ? 1 : 0;

        RELEASE_DIRSTR(WRITER, dir1);
    }

    return rc;
}

/* test directory existence (returns 'false' if a file) */
bool dir_exists(const char *dirname)
{
    DEBUGF("%s:dirname='%s'\n", __func__, dirname);

    int rc;

    file_internal_lock_WRITER();
    rc = open_file_internal(dirname, FF_DIR, NULL, NULL);
    file_internal_unlock_WRITER();

    SET_ERRNO_RC(rc);
    return FSI_S_OPENED(rc);
}

/* get the portable info from the native entry */
struct dirinfo dir_get_info(DIR *dirp, struct dirent *entry)
{
    int rc;

    if (!dirp || !entry)
        FILE_ERROR_RC(-EFAULT);

    if (entry->d_name[0] == '\0')
        FILE_ERROR_RC(-ENOENT);

    struct dirinfo dirinfo;
    rc = __FILEOP(convert_entry)(&dirinfo, entry,
                                 CVTENT_SRC_DIRENT | CVTENT_DST_DIRINFO);
    if (rc < 0)
        FILE_ERROR_RC(RC);

    return dirinfo;
file_error:
    DEBUGF("%s failed:%d\n", __func__, rc);
    SET_ERRNO_RC(rc);
    return (struct dirinfo){ .attribute = 0 };
}
