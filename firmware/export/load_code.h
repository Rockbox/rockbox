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


#ifndef __LOAD_CODE_H__
#define __LOAD_CODE_H__

#include "config.h"

extern void *lc_open(const char *filename, unsigned char *buf, size_t buf_size);

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
#include "system.h"

/* header is always at the beginning of the blob, and handle actually points
 * to the start of the blob (the header is there) */
static inline void *lc_open_from_mem(void* addr, size_t blob_size)
{
    (void)blob_size;
    /* commit dcache and discard icache */
    commit_discard_idcache();
    return addr;
}
static inline void *lc_get_header(void *handle) { return handle; }
/* no need to do anything */
static inline void lc_close(void *handle) { (void)handle; }

#elif (CONFIG_PLATFORM & PLATFORM_HOSTED)

extern void *lc_open_from_mem(void* addr, size_t blob_size);
extern void *lc_get_header(void *handle);
extern void  lc_close(void *handle);

#endif

/* this struct needs to be the first part of other headers
 * (lc_open() casts the other header to this one to load to the correct
 * address)
 */
struct lc_header {
    unsigned long magic;
    unsigned short target_id;
    unsigned short api_version;
    unsigned char *load_addr;
    unsigned char *end_addr;
};

#endif /* __LOAD_CODE_H__ */
