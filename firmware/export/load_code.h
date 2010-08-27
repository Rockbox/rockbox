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

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
#include "system.h"

extern void *lc_open(const char *filename, char *buf, size_t buf_size);
/* header is always at the beginning of the blob, and handle actually points
 * to the start of the blob */
static inline char *lc_open_from_mem(void* addr, size_t blob_size)
{
    (void)blob_size;
    cpucache_invalidate();
    return addr;
}
static inline void *lc_get_header(void *handle) { return handle; }
/* no need to do anything */
static inline void lc_close(void *handle) { (void)handle; }

#elif (CONFIG_PLATFORM & PLATFORM_HOSTED)

/* don't call these directly for loading code
 * they're to be wrapped by platform specific functions */
#ifdef WIN32
/* windows' LoadLibrary can only handle ucs2, no utf-8 */
#define _lc_open_char wchar_t
#else
#define _lc_open_char char
#endif
extern void *_lc_open(const _lc_open_char *filename, char *buf, size_t buf_size);
extern void *_lc_get_header(void *handle);
extern void  _lc_close(void *handle);

extern void *lc_open(const char *filename, char *buf, size_t buf_size);
/* not possiible on hosted platforms */
extern void *lc_open_from_mem(void *addr, size_t blob_size);
extern void *lc_get_header(void *handle);
extern void  lc_close(void *handle);
extern const char* lc_last_error(void);
#endif
