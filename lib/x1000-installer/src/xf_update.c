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

#include "xf_update.h"
#include "xf_error.h"
#include "file.h"
#include "md5.h"
#include <string.h>

static int process_stream(struct xf_nandio* nio,
                          struct xf_map* map,
                          struct xf_stream* stream)
{
    void* buffer;
    size_t count;
    int rc;

    /* calculate MD5 on the fly if taking a backup */
    md5_context md5_ctx;
    if(nio->mode == XF_NANDIO_READ)
        md5_starts(&md5_ctx);

    /* first deal with the file data */
    size_t bytes_left = map->length;
    while(bytes_left > 0) {
        count = bytes_left;
        rc = xf_nandio_get_buffer(nio, &buffer, &count);
        if(rc)
            return rc;

        if(nio->mode == XF_NANDIO_READ) {
            md5_update(&md5_ctx, buffer, count);
            rc = xf_stream_write(stream, buffer, count);
        } else {
            rc = xf_stream_read(stream, buffer, count);
        }

        bytes_left -= count;

        if(rc < 0 || (size_t)rc > count)
            return XF_E_IO;

        if((size_t)rc < count) {
            /* backup - we could not write all the data */
            if(nio->mode == XF_NANDIO_READ)
                return XF_E_IO;

            /* update - clear rest of buffer to 0xff */
            memset(buffer + rc, 0xff, count - rc);
            break;
        }
    }

    /* if updating - write blanks to the remainder of the region */
    while(bytes_left > 0) {
        count = bytes_left;
        rc = xf_nandio_get_buffer(nio, &buffer, &count);
        if(rc)
            return rc;

        memset(buffer, 0xff, count);
        bytes_left -= count;
    }

    /* finalize the MD5 sum */
    if(nio->mode == XF_NANDIO_READ) {
        md5_finish(&md5_ctx, map->md5);
        map->file_length = map->length;
        map->flags |= XF_MAP_HAS_MD5 | XF_MAP_HAS_FILE_LENGTH;
    }

    return XF_E_SUCCESS;
}

static int process_map(struct xf_nandio* nio, struct xf_map* map, int map_size,
                       xf_update_open_stream_cb open_stream, void* os_arg)
{
    int rc, rc2;
    struct xf_stream stream;

    /* ensure the map is sequential and non-overlapping before continuing */
    if(xf_map_validate(map, map_size) != 0)
        return XF_E_INVALID_PARAMETER;

    for(int i = 0; i < map_size; ++i) {
        /* seek to initial offset */
        rc = xf_nandio_seek(nio, map[i].offset);
        if(rc)
            return rc;

        rc = open_stream(os_arg, map[i].name, &stream);
        if(rc)
            return XF_E_CANNOT_OPEN_FILE;

        /* process the stream and be sure to close it even on error */
        rc = process_stream(nio, &map[i], &stream);
        rc2 = xf_stream_close(&stream);

        /* bail if either operation raised an error */
        if(rc)
            return rc;
        if(rc2)
            return rc2;
    }

    /* now flush to ensure all data was written */
    rc = xf_nandio_flush(nio);
    if(rc)
        return rc;

    return XF_E_SUCCESS;
}

static const enum xf_nandio_mode update_mode_to_nandio[] = {
    [XF_UPDATE] = XF_NANDIO_PROGRAM,
    [XF_BACKUP] = XF_NANDIO_READ,
    [XF_VERIFY] = XF_NANDIO_VERIFY,
};

int xf_updater_run(enum xf_update_mode mode, struct xf_nandio* nio,
                   struct xf_map* map, int map_size,
                   xf_update_open_stream_cb open_stream, void* arg)
{
    /* Switch NAND I/O into the correct mode */
    int rc = xf_nandio_set_mode(nio, update_mode_to_nandio[mode]);
    if(rc)
        return rc;

    /* This does all the real work */
    return process_map(nio, map, map_size, open_stream, arg);
}
