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

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
/* load binary blob from disk to memory, returning a handle */
void * lc_open(const char *filename, char *buf, size_t buf_size)
{
    int fd = open(filename, O_RDONLY);
    ssize_t read_size;

    if (fd < 0)
    {
        DEBUGF("Could not open file");
        return NULL;
    }

#if NUM_CORES > 1
    /* Make sure COP cache is flushed and invalidated before loading */
    {
        int my_core = switch_core(CURRENT_CORE ^ 1);
        cpucache_invalidate();
        switch_core(my_core);
    }
#endif

    read_size = read(fd, buf, buf_size);
    close(fd);
    cpucache_invalidate();

    if (read_size < 0)
    {
        DEBUGF("Could not read from file");
        return NULL;
    }
    return buf;
}

#elif (CONFIG_PLATFORM & PLATFORM_HOSTED)
/* libdl wrappers */


#ifdef WIN32
/* win32 */
#include <windows.h>
#define dlopen(_x_, _y_) LoadLibraryW(_x_)
#define dlsym(_x_, _y_) (void *)GetProcAddress(_x_, _y_)
#define dlclose(_x_) FreeLibrary(_x_)
static inline char *_dlerror(void)
{
    static char err_buf[64];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0,
                   err_buf, sizeof(err_buf), NULL);
    return err_buf;
}
#define dlerror _dlerror
#else
/* unix */
#include <dlfcn.h>
#define O_BINARY 0
#endif
#include <stdio.h>
#include "rbpaths.h"
#include "general.h"

void * _lc_open(const char *filename, char *buf, size_t buf_size)
{
    (void)buf;
    (void)buf_size;
    void* dl_handle = dlopen(filename, RTLD_NOW);
    return dl_handle;
}

void *lc_open_from_mem(void *addr, size_t blob_size)
{
    int fd, i;
    char temp_filename[MAX_PATH];

    /* We have to create the dynamic link library file from ram so we
       can simulate the codec loading. With voice and crossfade,
       multiple codecs may be loaded at the same time, so we need
       to find an unused filename */
    for (i = 0; i < 10; i++)
    {
#if (CONFIG_PLATFORM & PLATFORM_ANDROID)
        /* we need that path fixed, since get_user_file_path()
         * gives us the folder on the sdcard where we cannot load libraries
         * from (no exec permissions)
         */
        snprintf(temp_filename, sizeof(temp_filename),
                 "/data/data/org.rockbox/app_rockbox/libtemp_binary_%d.so", i);
#else
        char name[MAX_PATH];
        const char *_name = get_user_file_path(ROCKBOX_DIR, NEED_WRITE, name, sizeof(name));
        snprintf(temp_filename, sizeof(temp_filename),
                 "%slibtemp_binary_%d.dll", _name, i);
#endif
        fd = open(temp_filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0766);
        if (fd >= 0)
            break;  /* Created a file ok */
    }

    DEBUGF("Creating %s\n", temp_filename);
    if (fd < 0)
    {
        DEBUGF("open failed\n");
        return NULL;
    }

    if (write(fd, addr, blob_size) < (ssize_t)blob_size)
    {
        DEBUGF("Write failed\n");
        close(fd);
        remove(temp_filename);
        return NULL;
    }

    close(fd);
    return lc_open(temp_filename, NULL, 0);
}


void *_lc_get_header(void *handle)
{
    char *ret = dlsym(handle, "__header");
    if (ret == NULL)
        ret = dlsym(handle, "___header");

    return ret;
}

void _lc_close(void *handle)
{
    if (handle)
        dlclose(handle);
}

const char *lc_last_error(void)
{
    return dlerror();
}
#endif
