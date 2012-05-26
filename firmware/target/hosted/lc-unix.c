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

#include <string.h> /* size_t */
#include <dlfcn.h>
#include "debug.h"
#include "load_code.h"

void *lc_open(const char *filename, unsigned char *buf, size_t buf_size)
{
    (void)buf;
    (void)buf_size;
    void *handle = dlopen(filename, RTLD_NOW);
    if (handle == NULL)
    {
        DEBUGF("failed to load %s\n", filename);
        DEBUGF("lc_open(%s): %s\n", filename, dlerror());
    }
    return handle;
}

void *lc_get_header(void *handle)
{
    char *ret = dlsym(handle, "__header");
    if (ret == NULL)
        ret = dlsym(handle, "___header");

    return ret;
}

void lc_close(void *handle)
{
    dlclose(handle);
}

void *lc_open_from_mem(void *addr, size_t blob_size)
{
    (void)addr;
    (void)blob_size;
    /* we don't support loading code from memory on application builds,
     * it doesn't make sense (since it means writing the blob to disk again and
     * then falling back to load from disk) and requires the ability to write
     * to an executable directory */
    return NULL;
}
