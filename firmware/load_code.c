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

#include "config.h"
#include "system.h"
#include "file.h"
#include "debug.h"
#include "load_code.h"

/* load binary blob from disk to memory, returning a handle */
void * lc_open(const char *filename, unsigned char *buf, size_t buf_size)
{
    int fd = open(filename, O_RDONLY);
    ssize_t read_size;
    struct lc_header hdr;
    unsigned char *buf_end = buf+buf_size;
    off_t copy_size;

    if (fd < 0)
    {
        DEBUGF("Could not open file");
        goto error;
    }

#if NUM_CORES > 1
    /* Make sure COP cache is flushed and invalidated before loading */
    {
        int my_core = switch_core(CURRENT_CORE ^ 1);
        switch_core(my_core);
    }
#endif

    /* read the header to obtain the load address */
    read_size = read(fd, &hdr, sizeof(hdr));

    if (read_size < 0)
    {
        DEBUGF("Could not read from file");
        goto error_fd;
    }

    /* hdr.end_addr points to the end of the bss section,
     * but there might be idata/icode behind that so the bytes to copy
     * can be larger */
    copy_size = MAX(filesize(fd), hdr.end_addr - hdr.load_addr);

    if (hdr.load_addr < buf || (hdr.load_addr+copy_size) > buf_end)
    {
        DEBUGF("Binary doesn't fit into memory");
        goto error_fd;
    }

    /* go back to beginning to load the whole thing (incl. header) */
    if (lseek(fd, 0, SEEK_SET) < 0)
    {
        DEBUGF("lseek failed");
        goto error_fd;
    }

    /* the header has the addresses where the code is linked at */
    read_size = read(fd, hdr.load_addr, copy_size);
    close(fd);

    if (read_size < 0)
    {
        DEBUGF("Could not read from file");
        goto error;
    }

    /* commit dcache and discard icache */
    commit_discard_idcache();
    /* return a pointer the header, reused by lc_get_header() */
    return hdr.load_addr;

error_fd:
    close(fd);
error:
    return NULL;
}
