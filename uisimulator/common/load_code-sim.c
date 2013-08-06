/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Daniel Stenberg
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
#include <stdio.h>
#include "config.h"
#include "system.h"
#include "file.h"
#include "load_code.h"
#include "rbpaths.h"
#include "debug.h"

void * lc_open(const char *filename, unsigned char *buf, size_t buf_size)
{
    char osfilename[SIM_TMPBUF_MAX_PATH];
    if (sim_get_os_path(osfilename, filename, sizeof (osfilename)) < 0)
        return NULL;

    return os_lc_open(osfilename);
    (void)buf; (void)buf_size;
}

void * lc_open_from_mem(void *addr, size_t blob_size)
{
    /* We have to create the dynamic link library file from ram so we can
       simulate code loading. Multiple binaries may be loaded at the same
       time, so we need to find an unused filename. */
    int fd;
    char tempfile[SIM_TMPBUF_MAX_PATH];
    for (unsigned int i = 0; i < 10; i++)
    {
        snprintf(tempfile, sizeof(tempfile),
                 ROCKBOX_DIR "/libtemp_binary_%d.dll", i);
        fd = sim_open(tempfile, O_WRONLY|O_CREAT|O_TRUNC, 0700);
        if (fd >= 0)
            break;  /* Created a file ok */
    }

    if (fd < 0)
    {
        DEBUGF("open failed\n");
        return NULL;
    }

    if (sim_write(fd, addr, blob_size) != (ssize_t)blob_size)
    {
        DEBUGF("Write failed\n");
        sim_close(fd);
        sim_remove(tempfile);
        return NULL;
    }

    sim_close(fd);
    return lc_open(tempfile, NULL, 0);
}
