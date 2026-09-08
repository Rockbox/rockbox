/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

/* Reading members out of a Rockbox boot package. The package layout is the
 * same whatever the SoC is, so the per-CPU boot code shares this. */

#include "jztool.h"
#include "jztool_private.h"
#include "microtar-stdio.h"
#include <string.h>

int jz_boot_get_file(jz_context* jz, struct mtar* tar, const char* file,
                     unsigned int flags, jz_buffer** buf)
{
    jz_buffer* buffer = NULL;
    const mtar_header_t* h;
    int rc;

    rc = mtar_find(tar, file);
    if(rc != MTAR_ESUCCESS) {
        if(!(flags & JZ_BOOT_OPTIONAL))
            jz_log(jz, JZ_LOG_ERROR, "can't find %s in boot file, tar error %d",
                   file, rc);
        return JZ_ERR_OPEN_FILE;
    }

    h = mtar_get_header(tar);
    buffer = jz_buffer_alloc(h->size, NULL);
    if(!buffer)
        return JZ_ERR_OUT_OF_MEMORY;

    rc = mtar_read_data(tar, buffer->data, buffer->size);
    if(rc < 0 || (unsigned)rc != buffer->size) {
        jz_buffer_free(buffer);
        jz_log(jz, JZ_LOG_ERROR, "can't read %s in boot file, tar error %d",
               file, rc);
        return JZ_ERR_BAD_FILE_FORMAT;
    }

    if(flags & JZ_BOOT_DECOMPRESS) {
        uint32_t dst_len;
        jz_buffer* nbuf = jz_ucl_unpack(buffer->data, buffer->size, &dst_len);
        jz_buffer_free(buffer);
        if(!nbuf) {
            jz_log(jz, JZ_LOG_ERROR, "error decompressing %s in boot file", file);
            return JZ_ERR_BAD_FILE_FORMAT;
        }

        /* for simplicity just forget original size of buffer */
        nbuf->size = dst_len;
        buffer = nbuf;
    }

    *buf = buffer;
    return JZ_SUCCESS;
}

int jz_boot_show_version(jz_context* jz, jz_buffer* info_file)
{
    /* Extract the version string and log it for informational purposes */
    char* boot_version = (char*)info_file->data;
    char* endpos = memchr(boot_version, '\n', info_file->size);
    if(!endpos) {
        jz_log(jz, JZ_LOG_ERROR, "invalid metadata in boot file");
        return JZ_ERR_BAD_FILE_FORMAT;
    }

    *endpos = 0;
    jz_log(jz, JZ_LOG_NOTICE, "Rockbox bootloader version: %s", boot_version);
    return JZ_SUCCESS;
}
