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
#include <stdlib.h>
#include <string.h>
#include "system.h"
#include "load_code.h"
#include "debug.h"
#include "codecs.h"
#include "fs_defines.h"
#include "sys_file.h"
#include <pspkernel.h>

#define DEBUG_PRX_MEMORY

/* Define LOGF_ENABLE to enable logf output in this file */
#define LOGF_ENABLE
#include "logf.h"

struct _prx_handle {
    SceUID id;
};

void * lc_open(const char *filename, unsigned char *buf, size_t buf_size)
{
    logf("lc_open(path=\"%s\")\n", filename);

    SceKernelLMOption option;
    SceUID mpid = PSP_MEMORY_PARTITION_USER;

    struct _prx_handle *handle = (struct _prx_handle *) malloc(sizeof(struct _prx_handle));
    if (handle == NULL) {
        return NULL;
    }

    handle->id = -1;
    memset(&option, 0, sizeof(option));

    option.size = sizeof(option);
    option.mpidtext = mpid;
    option.mpiddata = mpid;
    option.position = 0;
    option.access = 1;

#ifdef DEBUG_PRX_MEMORY
    SceSize am = sceKernelTotalFreeMemSize();
    logf("available memory before loading: %d bytes (%dKB or %dMB)",
        am, am / 1024, am / 1024 / 1024 );
#endif

    char realpath[1024];
    const char *fpath = handle_special_dirs(filename, 0x0, realpath,
                                            sizeof (realpath));

    handle->id = sceKernelLoadModule(fpath, 0, &option);

#ifdef DEBUG_PRX_MEMORY
    am = sceKernelTotalFreeMemSize();
    logf("available memory after loading: %d bytes (%dKB or %dMB)",
        am, am / 1024, am / 1024 / 1024 );
#endif
    sceKernelDelayThread(100*100);

    if (handle->id <= 0) {
        DEBUGF("sceKernelLoadModule: error 0x%x\n", handle->id);
        return NULL;
    }

    return (void *) handle;
    (void) buf; (void) buf_size;
}

extern void *plugin_get_header(void);
void * lc_get_header(void *handle)
{
    struct _prx_handle *h = (struct _prx_handle *) handle;

    logf("lc_get_header(handle=%p, id=0x%x)\n", h, h->id);

    int fd;
    int result = sceKernelStartModule(h->id, 0, NULL, &fd, NULL);
    if (result <= 0) {
        logf("sceKernelStartModule: error 0x%x\n", result);
        return NULL;
    }

    sceKernelDelayThread(100*100);

    logf("sceKernelStartModule(%d) success 0x%x\n", fd, result);

    return plugin_get_header();
}

void lc_close(void *handle)
{
    struct _prx_handle *h = (struct _prx_handle *) handle;
    if (h != NULL) {
        int status;
        int result = sceKernelStopModule( h->id, 0, NULL, &status, NULL );
        if (result < 0) {
            logf("sceKernelStopModule: error 0x%x\n", result);
        }

        result = sceKernelUnloadModule( h->id );
        if (result < 0) {
            logf("sceKernelUnloadModule: error 0x%x\n", result);
        }

        free(h);
    }
}
