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
#ifndef __LC_ROCK_H__
#define __LC_ROCK_H__

#include "system.h"

void *lc_open(const char *filename, unsigned char *buf, size_t buf_size);

#if defined(HAVE_LC_OPEN_FROM_MEM)
/* header is always at the beginning of the blob, and handle actually points
 * to the start of the blob (the header is there) */
static inline void *lc_open_from_mem(void* addr, size_t blob_size)
{
    (void)blob_size;
    /* commit dcache and discard icache */
    commit_discard_idcache();
    return addr;
}
#endif /* HAVE_LC_OPEN_FROM_MEM */

static inline void *lc_get_header(void *handle)
{
    return handle;
}

/* no need to do anything */
static inline void lc_close(void *handle)
{
    (void)handle;
}

#endif /* __LC_ROCK_H__ */
