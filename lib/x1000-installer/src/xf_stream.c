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

#include "xf_stream.h"
#include "xf_error.h"
#include "file.h"
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

/*
 * File streams
 */

static off_t file_stream_get_size(struct xf_stream* s)
{
    return filesize(s->data);
}

static ssize_t file_stream_read(struct xf_stream* s, void* buf, size_t len)
{
    return read(s->data, buf, len);
}

static ssize_t file_stream_write(struct xf_stream* s, const void* buf, size_t len)
{
    return write(s->data, buf, len);
}

static int file_stream_close(struct xf_stream* s)
{
    return close(s->data);
}

static const struct xf_stream_ops file_stream_ops = {
    .xget_size = file_stream_get_size,
    .xread = file_stream_read,
    .xwrite = file_stream_write,
    .xclose = file_stream_close,
};

int xf_open_file(const char* file, int flags, struct xf_stream* s)
{
    s->data = open(file, flags, 0666);
    s->ops = &file_stream_ops;
    return (s->data >= 0) ? 0 : -1;
}

/*
 * Tar streams
 */

static off_t tar_stream_get_size(struct xf_stream* s)
{
    mtar_t* mtar = (mtar_t*)s->data;
    return mtar_get_header(mtar)->size;
}

static ssize_t tar_stream_read(struct xf_stream* s, void* buffer, size_t count)
{
    mtar_t* mtar = (mtar_t*)s->data;

    int ret = mtar_read_data(mtar, buffer, count);
    if(ret < 0)
        return -1;

    return ret;
}

static ssize_t tar_stream_write(struct xf_stream* s, const void* buffer, size_t count)
{
    mtar_t* mtar = (mtar_t*)s->data;

    int ret = mtar_write_data(mtar, buffer, count);
    if(ret < 0)
        return -1;

    return ret;
}

static int tar_stream_close(struct xf_stream* s)
{
    mtar_t* mtar = (mtar_t*)s->data;

    if(mtar_access_mode(mtar) == MTAR_WRITE) {
        if(mtar_update_file_size(mtar) != MTAR_ESUCCESS)
            return -1;
        if(mtar_end_data(mtar) != MTAR_ESUCCESS)
            return -1;
    }

    return 0;
}

static const struct xf_stream_ops tar_stream_ops = {
    .xget_size = tar_stream_get_size,
    .xread = tar_stream_read,
    .xwrite = tar_stream_write,
    .xclose = tar_stream_close,
};

int xf_open_tar(mtar_t* mtar, const char* file, struct xf_stream* s)
{
    int err = mtar_find(mtar, file);
    if(err != MTAR_ESUCCESS)
        return err;

    /* must only read normal files */
    const mtar_header_t* h = mtar_get_header(mtar);
    if(h->type != 0 && h->type != MTAR_TREG)
        return MTAR_EFAILURE;

    s->data = (intptr_t)mtar;
    s->ops = &tar_stream_ops;
    return MTAR_ESUCCESS;
}

int xf_create_tar(mtar_t* mtar, const char* file, size_t size, struct xf_stream* s)
{
    int err = mtar_write_file_header(mtar, file, size);
    if(err)
        return err;

    s->data = (intptr_t)mtar;
    s->ops = &tar_stream_ops;
    return MTAR_ESUCCESS;
}

/*
 * Utility functions
 */

int xf_stream_read_lines(struct xf_stream* s, char* buf, size_t bufsz,
                         int(*callback)(int n, char* buf, void* arg), void* arg)
{
    char* startp, *endp;
    ssize_t bytes_read;
    int rc;

    int n = 0;
    size_t pos = 0;
    bool at_eof = false;

    if(bufsz <= 1)
        return XF_E_LINE_TOO_LONG;

    while(!at_eof) {
        bytes_read = xf_stream_read(s, &buf[pos], bufsz - pos - 1);
        if(bytes_read < 0)
            return XF_E_IO;

        /* short read is end of file */
        if((size_t)bytes_read < bufsz - pos - 1)
            at_eof = true;

        pos += bytes_read;
        buf[pos] = '\0';

        startp = endp = buf;
        while(endp != &buf[pos]) {
            endp = strchr(startp, '\n');
            if(endp) {
                *endp = '\0';
            } else {
                if(!at_eof) {
                    if(startp == buf)
                        return XF_E_LINE_TOO_LONG;
                    else
                        break; /* read ahead to look for newline */
                } else {
                    if(startp == &buf[pos])
                        break; /* nothing left to do */
                    else
                        endp = &buf[pos]; /* last line missing a newline -
                                           * treat EOF as newline */
                }
            }

            rc = callback(n++, startp, arg);
            if(rc != 0)
                return rc;

            startp = endp + 1;
        }

        if(startp <= &buf[pos]) {
            memmove(buf, startp, &buf[pos] - startp);
            pos = &buf[pos] - startp;
        }
    }

    return 0;
}
