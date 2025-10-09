/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Mauricio G.
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
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include "system.h"
#include "load_code.h"
#include "filesystem-ctru.h"
#include "debug.h"

void* programResolver(const char* sym, void *userData);
void * lc_open(const char *filename, unsigned char *buf, size_t buf_size)
{
    DEBUGF("dlopen(path=\"%s\")\n", filename);

    /* note: the 3ds dlopen implementation needs a custom resolver
       for the unresolved symbols in shared objects */
    /* void *handle = dlopen(filename, RTLD_NOW | RTLD_LOCAL); */
    void *handle = ctrdlOpen(filename,
                             RTLD_NOW | RTLD_LOCAL,
                             programResolver,
                             NULL);
    if (handle == NULL)
    {
        DEBUGF("%s(\"%s\") failed\n", __func__, filename);
        DEBUGF("  lc_open error '%s'\n", dlerror());
    }
    DEBUGF("handle = %p\n", handle);

    return handle;
    (void) buf; (void) buf_size;
}

void * lc_get_header(void *handle)
{
    void *symbol = dlsym(handle, "__header");
    if (!symbol) {
        symbol = dlsym(handle, "___header");
    }

    return symbol;
}

void lc_close(void *handle)
{
    if (handle) {
        dlclose(handle);
    }
}
