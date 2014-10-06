 /***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Port of xrick, a Rick Dangerous clone, to Rockbox.
 * See http://www.bigorno.net/xrick/
 *
 * Copyright (C) 2008-2014 Pierluigi Vicinanza
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

#include "xrick/system/system.h"

#include "xrick/config.h"
#include "xrick/util.h"

#include "plugin.h"

#define XRICK_GAME_DIR PLUGIN_GAMES_DATA_DIR "/xrick"

/*
 * Global variables
 */
const char *sysfile_defaultPath = XRICK_GAME_DIR;

/*
 * Local variables
 */
static char *rootPath = NULL;

/*
 *
 */
bool sysfile_setRootPath(const char *name)
{
    rootPath = u_strdup(name);
    return (rootPath != NULL);
}

/*
 *
 */
void sysfile_clearRootPath()
{
    sysmem_pop(rootPath);
    rootPath = NULL;
}

/*
 * Open a data file.
 */
file_t sysfile_open(const char *name)
{
    int fd;

    size_t fullPathLength = rb->strlen(rootPath) + rb->strlen(name) + 2;
    char *fullPath = sysmem_push(fullPathLength);
    if (!fullPath)
    {
        return NULL;
    }
    rb->snprintf(fullPath, fullPathLength, "%s/%s", rootPath, name);
    fd = rb->open(fullPath, O_RDONLY);
    sysmem_pop(fullPath);

    /*
     * note: I've never seen zero/NULL being used as a file descriptor under Rockbox.
     *       Putting a check here in case this will ever happen (will need a fix).
     */
    if (fd == 0)
    {
        sys_error("(file) unsupported file descriptor (zero/NULL) being used");
    }
    if (fd < 0)
    {
        return NULL;
    }

    return (file_t)fd;
}

/*
 * Read a file within a data archive.
 */
int sysfile_read(file_t file, void *buf, size_t size, size_t count)
{
    int fd = (int)file;
    return (rb->read(fd, buf, size * count) / size);
}

/*
 * Seek.
 */
int sysfile_seek(file_t file, long offset, int origin)
{
    int fd = (int)file;
    return rb->lseek(fd, offset, origin);
}

/*
 * Close a file within a data archive.
 */
void sysfile_close(file_t file)
{
    int fd = (int)file;
    rb->close(fd);
}

/* eof */
