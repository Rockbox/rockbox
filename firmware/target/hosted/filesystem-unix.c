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
#include <sys/statvfs.h>
#include <sys/stat.h>
#include "config.h"
#include "system.h"
#include "file.h"
#include "dir.h"
#include "mv.h"
#include "debug.h"

off_t os_filesize(int osfd)
{
    struct stat buf;

    if (!os_fstat(osfd, &buf))
        return buf.st_size;
    else
        return -1;
}

int os_fsamefile(int osfd1, int osfd2)
{
    struct stat s1, s2;

    if (os_fstat(osfd1, &s1))
        return -1;

    if (os_fstat(osfd2, &s2))
        return -1;

    return s1.st_dev == s2.st_dev && s1.st_ino == s2.st_ino ? 1 : 0;
}

bool os_file_exists(const char *ospath)
{
    int sim_fd = os_open(ospath, O_RDONLY, 0);
    if (sim_fd < 0)
        return false;

    os_close(sim_fd);
    return true;
}

int os_opendirfd(const char *osdirname)
{
    return os_open(osdirname, O_RDONLY);
}

int os_getdirfd(const char *osdirname, DIR *osdirp, bool *is_opened)
{
    *is_opened = false;

    int fd = os_dirfd(osdirp);
    if (fd < 0)
    {
        fd = os_opendirfd(osdirname);
        if (fd >= 0)
            *is_opened = true;
    }

    return fd;
}

/* do we really need this in the app? (in the sim, yes) */
void volume_size(IF_MV(int volume,) unsigned long *sizep, unsigned long *freep)
{
    unsigned long size = 0, free = 0;
    char volpath[MAX_PATH];
    struct statvfs vfs;

    if (os_volume_path(IF_MV(volume,) volpath, sizeof (volpath)) >= 0
        && !statvfs(volpath, &vfs))
    {
        DEBUGF("statvfs: frsize=%d blocks=%ld bfree=%ld\n",
               (int)vfs.f_frsize, (long)vfs.f_blocks, (long)vfs.f_bfree);
        if (sizep)
            size = (vfs.f_blocks / 2) * (vfs.f_frsize / 512);

        if (freep)
            free = (vfs.f_bfree / 2) * (vfs.f_frsize / 512);
    }

    if (sizep)
        *sizep = size;

    if (freep)
        *freep = free;
}
