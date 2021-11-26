/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "file.h"
#include "microtar-rockbox.h"
#include <stdint.h>

static int fd_read(void* stream, void* data, unsigned size)
{
    ssize_t res = read((intptr_t)stream, data, size);
    if(res < 0)
        return MTAR_EREADFAIL;

    return res;
}

static int fd_write(void* stream, const void* data, unsigned size)
{
    ssize_t res = write((intptr_t)stream, data, size);
    if(res < 0)
        return MTAR_EWRITEFAIL;

    return res;
}

static int fd_seek(void* stream, unsigned offset)
{
    off_t res = lseek((intptr_t)stream, offset, SEEK_SET);
    if(res < 0 || ((unsigned)res != offset))
        return MTAR_ESEEKFAIL;
    else
        return MTAR_ESUCCESS;
}

static int fd_close(void* stream)
{
    close((intptr_t)stream);
    return MTAR_ESUCCESS;
}

static const mtar_ops_t fd_ops = {
    .read = fd_read,
    .write = fd_write,
    .seek = fd_seek,
    .close = fd_close,
};

int mtar_open(mtar_t* tar, const char* filename, int flags)
{
    /* Determine access mode */
    int access;
    switch(flags & O_ACCMODE) {
    case O_RDONLY:
        access = MTAR_READ;
        break;

    case O_WRONLY:
        access = MTAR_WRITE;
        break;

    default:
        /* Note - O_RDWR isn't very useful so we don't allow it */
        return MTAR_EAPI;
    }

    int fd = open(filename, flags);
    if(fd < 0)
        return MTAR_EOPENFAIL;

    /* Init tar struct and functions */
    mtar_init(tar, access, &fd_ops, (void*)(intptr_t)fd);
    return MTAR_ESUCCESS;
}
