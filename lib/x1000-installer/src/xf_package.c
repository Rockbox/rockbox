/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

#include "xf_package.h"
#include "xf_error.h"
#include "pathfuncs.h"
#include "file.h"
#include "core_alloc.h"
#include "md5.h"
#include "system.h"
#include <stdbool.h>
#include <string.h>

#ifdef ROCKBOX
# include "microtar-rockbox.h"
#else
# include "microtar-stdio.h"
#endif

#define METADATA_SIZE   4096    /* size of the metadata buffer */
#define MAX_MAP_SIZE    32      /* maximum number of map entries */

static int pkg_alloc(struct xf_package* pkg)
{
    memset(pkg, 0, sizeof(*pkg));

    /* calculate allocation size */
    size_t alloc_size = 0;
    alloc_size += ALIGN_UP_P2(sizeof(mtar_t), 3);
    alloc_size += ALIGN_UP_P2(sizeof(struct xf_map) * MAX_MAP_SIZE, 3);
    alloc_size += ALIGN_UP_P2(METADATA_SIZE, 3);
    alloc_size += 7; /* for alignment */

    pkg->alloc_handle = core_alloc_ex("xf_package", alloc_size, &buflib_ops_locked);
    if(pkg->alloc_handle < 0)
        return XF_E_OUT_OF_MEMORY;

    /* distribute memory */
    uint8_t* buf = (uint8_t*)core_get_data(pkg->alloc_handle);
    memset(buf, 0, alloc_size);
    ALIGN_BUFFER(buf, alloc_size, 8);

    pkg->tar = (mtar_t*)buf;
    buf += ALIGN_UP_P2(sizeof(mtar_t), 3);

    pkg->map = (struct xf_map*)buf;
    buf += ALIGN_UP_P2(sizeof(struct xf_map) * MAX_MAP_SIZE, 3);

    pkg->metadata = (char*)buf;
    buf += ALIGN_UP_P2(METADATA_SIZE, 3);

    return XF_E_SUCCESS;
}

static int read_meta_line_cb(int n, char* buf, void* arg)
{
    struct xf_package* pkg = (struct xf_package*)arg;
    size_t length = strlen(buf);

    /* skip blank lines and the first line (it's reserved for old format) */
    if(n == 0 || length == 0)
        return 0;

    /* metadata lines require an '=' sign to separate key and value */
    if(!strchr(buf, '='))
        return XF_E_SYNTAX_ERROR;

    /* we need space to copy the key-value pair plus a null terminator */
    if(length + 1 >= METADATA_SIZE - pkg->metadata_len)
        return XF_E_BUF_OVERFLOW;

    memcpy(&pkg->metadata[pkg->metadata_len], buf, length + 1);
    pkg->metadata_len += length + 1;
    return 0;
}

static int pkg_read_meta(struct xf_package* pkg)
{
    struct xf_stream stream;
    int rc = xf_open_tar(pkg->tar, "bootloader-info.txt", &stream);
    if(rc)
        return XF_E_CANNOT_OPEN_FILE;

    char buf[200];
    rc = xf_stream_read_lines(&stream, buf, sizeof(buf), read_meta_line_cb, pkg);
    xf_stream_close(&stream);
    return rc;
}

static int pkg_read_map(struct xf_package* pkg,
                        const struct xf_map* dflt_map, int dflt_map_size)
{
    /* Attempt to load and parse the map file */
    struct xf_stream stream;
    int rc = xf_open_tar(pkg->tar, "flashmap.txt", &stream);

    /* If the flash map is absent but a default map has been provided,
     * then the update is in the old fixed format. */
    if(rc == MTAR_ENOTFOUND && dflt_map) {
        if(dflt_map_size > MAX_MAP_SIZE)
            return XF_E_INVALID_PARAMETER;

        for(int i = 0; i < dflt_map_size; ++i) {
            pkg->map[i] = dflt_map[i];
            pkg->map[i].flags &= ~(XF_MAP_HAS_MD5 | XF_MAP_HAS_FILE_LENGTH);
        }

        pkg->map_size = dflt_map_size;
        return XF_E_SUCCESS;
    }

    if(rc != MTAR_ESUCCESS)
        return XF_E_CANNOT_OPEN_FILE;

    rc = xf_map_parse(&stream, pkg->map, MAX_MAP_SIZE);
    if(rc < 0)
        goto err;

    /* Sort the map; reject it if there is any overlap. */
    pkg->map_size = rc;
    if(xf_map_sort(pkg->map, pkg->map_size)) {
        rc = XF_E_INVALID_PARAMETER;
        goto err;
    }

    /* All packages in the 'new' format are required to have MD5 sums. */
    for(int i = 0; i < pkg->map_size; ++i) {
        if(!(pkg->map[i].flags & XF_MAP_HAS_MD5) ||
           !(pkg->map[i].flags & XF_MAP_HAS_FILE_LENGTH)) {
            rc = XF_E_VERIFY_FAILED;
            goto err;
        }
    }

    rc = XF_E_SUCCESS;

  err:
    xf_stream_close(&stream);
    return rc;
}

static int pkg_verify(struct xf_package* pkg)
{
    struct xf_stream stream;
    md5_context ctx;
    uint8_t buffer[128];

    for(int i = 0; i < pkg->map_size; ++i) {
        /* At a bare minimum, check that the file exists. */
        int rc = xf_open_tar(pkg->tar, pkg->map[i].name, &stream);
        if(rc)
            return XF_E_VERIFY_FAILED;

        /* Also check that it isn't bigger than the update region.
         * That would normally indicate a problem. */
        off_t streamsize = xf_stream_get_size(&stream);
        if(streamsize > (off_t)pkg->map[i].length) {
            rc = XF_E_VERIFY_FAILED;
            goto err;
        }

        /* Check against the listed file length. */
        if(pkg->map[i].flags & XF_MAP_HAS_FILE_LENGTH) {
            if(streamsize != (off_t)pkg->map[i].file_length) {
                rc = XF_E_VERIFY_FAILED;
                goto err;
            }
        }

        /* Check the MD5 sum if we have it. */
        if(pkg->map[i].flags & XF_MAP_HAS_MD5) {
            md5_starts(&ctx);
            while(1) {
                ssize_t n = xf_stream_read(&stream, buffer, sizeof(buffer));
                if(n < 0) {
                    rc = XF_E_IO;
                    goto err;
                }

                md5_update(&ctx, buffer, n);
                if((size_t)n < sizeof(buffer))
                    break;
            }

            md5_finish(&ctx, buffer);
            if(memcpy(buffer, pkg->map[i].md5, 16)) {
                rc = XF_E_VERIFY_FAILED;
                goto err;
            }
        }

      err:
        xf_stream_close(&stream);
        if(rc)
            return rc;
    }

    /* All files passed verification */
    return XF_E_SUCCESS;
}

int xf_package_open_ex(struct xf_package* pkg, const char* file,
                       const struct xf_map* dflt_map, int dflt_map_size)
{
    int rc = pkg_alloc(pkg);
    if(rc)
        return rc;

#ifdef ROCKBOX
    rc = mtar_open(pkg->tar, file, O_RDONLY);
#else
    rc = mtar_open(pkg->tar, file, "r");
#endif
    if(rc != MTAR_ESUCCESS) {
        rc = XF_E_CANNOT_OPEN_FILE;
        goto err;
    }

    rc = pkg_read_meta(pkg);
    if(rc)
        goto err;

    rc = pkg_read_map(pkg, dflt_map, dflt_map_size);
    if(rc)
        goto err;

    rc = pkg_verify(pkg);
    if(rc)
        goto err;

  err:
    if(rc)
        xf_package_close(pkg);
    return rc;
}

void xf_package_close(struct xf_package* pkg)
{
    if(mtar_is_open(pkg->tar))
        mtar_close(pkg->tar);

    if(pkg->alloc_handle > 0) {
        core_free(pkg->alloc_handle);
        pkg->alloc_handle = 0;
    }
}
