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

#include <stdlib.h>
#include <sys/stat.h> /* stat() */
#include <stdio.h>  /* snprintf */
#include <string.h> /* size_t */
#include <dirent.h>
#include <time.h>   /* localtime() */
#include "system-target.h"
#include "dir-target.h"
#include "file.h"
#include "dir.h"
#include "rbpaths.h"


long filesize(int fd)
{
    struct stat buf;

    if (!fstat(fd, &buf))
        return buf.st_size;
    else
        return -1;
}

/* do we really need this in the app? */
void fat_size(unsigned long* size, unsigned long* free)
{
    *size = *free = 0;
}

#undef opendir
#undef closedir
#undef mkdir
#undef readdir

/* need to wrap around DIR* because we need to save the parent's
 * directory path in order to determine dirinfo */
struct __dir {
    DIR  *dir;
    char *path;
};

DIR* _opendir(const char *name)
{
    char *buf = malloc(sizeof(struct __dir) + strlen(name)+1);
    if (!buf)
        return NULL;

    struct __dir *this = (struct __dir*)buf;

    this->path = buf+sizeof(struct __dir);
    /* definitely fits due to strlen() */
    strcpy(this->path, name);

    this->dir = opendir(name);

    if (!this->dir)
    {
        free(buf);
        return NULL;
    }
    return (DIR*)this;
}

int _mkdir(const char *name)
{
    return mkdir(name, 0777);
}

int _closedir(DIR *dir)
{
    struct __dir *this = (struct __dir*)dir;
    int ret = closedir(this->dir);
    free(this);
    return ret;
}

struct dirent* _readdir(DIR* dir)
{
    struct __dir *d = (struct __dir*)dir;
    return readdir(d->dir);
}

struct dirinfo dir_get_info(struct DIR* _parent, struct dirent *dir)
{
    struct __dir *parent = (struct __dir*)_parent;
    struct stat s;
    struct tm *tm = NULL;
    struct dirinfo ret;
    char path[MAX_PATH];

    snprintf(path, sizeof(path), "%s/%s", parent->path, dir->d_name);
    memset(&ret, 0, sizeof(ret));

    if (!stat(path, &s))
    {
        if (S_ISDIR(s.st_mode))
        {
            ret.attribute = ATTR_DIRECTORY;
        }
        ret.size = s.st_size;
        tm = localtime(&(s.st_mtime));
    }

    if (!lstat(path, &s) && S_ISLNK(s.st_mode))
    {
        ret.attribute |= ATTR_LINK;
    }

    if (tm)
    {
        ret.wrtdate = ((tm->tm_year - 80) << 9) |
                            ((tm->tm_mon + 1) << 5) |
                            tm->tm_mday;
        ret.wrttime = (tm->tm_hour << 11) |
                            (tm->tm_min << 5) |
                            (tm->tm_sec >> 1);
    }

    return ret;
}
