/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Thomas Martitz
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
#include <sys/statfs.h> /* lowest common denominator */
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <utime.h>
#include "config.h"
#include "system.h"
#include "file.h"
#include "dir.h"
#include "mv.h"
#include "debug.h"
#include "pathfuncs.h"
#include "string-extra.h"

#define SAME_FILE_INFO(sb1p, sb2p) \
    ((sb1p)->st_dev == (sb2p)->st_dev && (sb1p)->st_ino == (sb2p)->st_ino)

int os_modtime(const char *path, time_t modtime)
{
    struct utimbuf times =
    {
        .actime = modtime,
        .modtime = modtime,
    };

    return utime(path, &times);
}

off_t os_filesize(int osfd)
{
    struct stat sb;

    if (!os_fstat(osfd, &sb))
        return sb.st_size;
    else
        return -1;
}

int os_fsamefile(int osfd1, int osfd2)
{
    struct stat sb1, sb2;

    if (os_fstat(osfd1, &sb1))
        return -1;

    if (os_fstat(osfd2, &sb2))
        return -1;

    return SAME_FILE_INFO(&sb1, &sb2);
}

int os_relate(const char *ospath1, const char *ospath2)
{
    DEBUGF("\"%s\" : \"%s\"\n", ospath1, ospath2);

    if (!ospath2 || !*ospath2)
    {
        errno = ospath2 ? ENOENT : EFAULT;
        return -1;
    }

    /* First file must stay open for duration so that its stats don't change */
    int fd1 = os_open(ospath1, O_RDONLY | O_CLOEXEC);
    if (fd1 < 0)
        return -2;

    struct stat sb1;
    if (os_fstat(fd1, &sb1))
    {
        os_close(fd1);
        return -3;
    }

    char path2buf[strlen(ospath2) + 1];
    *path2buf = 0;

    ssize_t len = 0;
    const char *p = ospath2;
    const char *sepmo = path_is_absolute(ospath2) ? PA_SEP_HARD : PA_SEP_SOFT;

    int rc = RELATE_DIFFERENT;

    while (1)
    {
        if (sepmo != PA_SEP_HARD &&
            !(len = parse_path_component(&ospath2, &p)))
        {
            break;
        }

        char compname[len + 1];
        strmemcpy(compname, p, len);

        path_append(path2buf, sepmo, compname, sizeof (path2buf));
        sepmo = PA_SEP_SOFT;

        int errnum = errno; /* save and restore if not actually failing */
        struct stat sb2;

        if (!os_stat(path2buf, &sb2))
        {
            if (SAME_FILE_INFO(&sb1, &sb2))
            {
                rc = RELATE_SAME;
            }
            else if (rc == RELATE_SAME)
            {
                if (name_is_dot_dot(compname))
                    rc = RELATE_DIFFERENT;
                else if (!name_is_dot(compname))
                    rc = RELATE_PREFIX;
            }
        }
        else if (errno == ENOENT && !*GOBBLE_PATH_SEPCH(ospath2) &&
                 !name_is_dot_dot(compname))
        {
            if (rc == RELATE_SAME)
                rc = RELATE_PREFIX;

            errno = errnum;
            break;
        }
        else
        {
            rc = -4;
            break;
        }
    }

    if (os_close(fd1) && rc >= 0)
        rc = -5;

    return rc;
}

bool os_file_exists(const char *ospath)
{
    int sim_fd = os_open(ospath, O_RDONLY | O_CLOEXEC, 0);
    if (sim_fd < 0)
        return false;

    int errnum = errno;
    os_close(sim_fd);
    errno = errnum;

    return true;
}

int os_opendirfd(const char *osdirname)
{
    return os_open(osdirname, O_RDONLY | O_CLOEXEC);
}

int os_opendir_and_fd(const char *osdirname, DIR **osdirpp, int *osfdp)
{
    /* another possible way is to use open() then fdopendir() */
    *osdirpp = NULL;
    *osfdp = -1;

    DIR *dirp = os_opendir(osdirname);
    if (!dirp)
        return -1;

    int rc = 0;
    int errnum = errno;

    int fd = os_dirfd(dirp);
    if (fd < 0)
    {
        fd = os_opendirfd(osdirname);
        rc = 1;
    }

    if (fd < 0)
    {
        os_closedir(dirp);
        return -2;
    }

    errno = errnum;

    *osdirpp = dirp;
    *osfdp = fd;

    return rc;
}

/* do we really need this in the app? (in the sim, yes) */
void volume_size(IF_MV(int volume,) unsigned long *sizep, unsigned long *freep)
{
    unsigned long size = 0, free = 0;
    char volpath[MAX_PATH];
    struct statfs fs;

    if (os_volume_path(IF_MV(volume,) volpath, sizeof (volpath)) >= 0
        && !statfs(volpath, &fs))
    {
        DEBUGF("statvfs: frsize=%d blocks=%ld bfree=%ld\n",
               (int)fs.f_frsize, (long)fs.f_blocks, (long)fs.f_bfree);
        if (sizep)
            size = (fs.f_blocks / 2) * (fs.f_frsize / 512);

        if (freep)
            free = (fs.f_bfree / 2) * (fs.f_frsize / 512);
    }

    if (sizep)
        *sizep = size;

    if (freep)
        *freep = free;
}
