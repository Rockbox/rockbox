/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2022 Aidan MacDonald
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

#include "linuxboot.h"
#include "system.h"
#include "core_alloc.h"
#include "crc32.h"
#include "inflate.h"
#include "file.h"
#include <string.h>

/* compression support options - can be decided per target if needed,
 * for now default to enabling everything */
#define HAVE_UIMAGE_COMP_NONE
#define HAVE_UIMAGE_COMP_GZIP

uint32_t uimage_crc(uint32_t crc, const void* data, size_t size)
{
    /* is this endian swapping required...? */
    return letoh32(crc_32r(data, size, htole32(crc ^ 0xffffffff))) ^ 0xffffffff;
}

uint32_t uimage_calc_hcrc(const struct uimage_header* uh)
{
    struct uimage_header h = *uh;
    uimage_set_hcrc(&h, 0);
    return uimage_crc(0, &h, sizeof(h));
}

static int uimage_check_header(const struct uimage_header* uh)
{
    if(uimage_get_magic(uh) != IH_MAGIC)
        return -1;

    if(uimage_get_hcrc(uh) != uimage_calc_hcrc(uh))
        return -2;

    return 0;
}

static int uimage_alloc_state(const struct uimage_header* uh)
{
    size_t size;

    switch(uimage_get_comp(uh)) {
#ifdef HAVE_UIMAGE_COMP_NONE
    case IH_COMP_NONE:
        return 0;
#endif

#ifdef HAVE_UIMAGE_COMP_GZIP
    case IH_COMP_GZIP:
        size = inflate_size + inflate_align - 1;
        return core_alloc_ex("inflate", size, &buflib_ops_locked);
#endif

    default:
        return -1;
    }
}

#ifdef HAVE_UIMAGE_COMP_GZIP
struct uimage_inflatectx
{
    uimage_reader reader;
    void* rctx;
    uint32_t dcrc;
    size_t remain;
};

static uint32_t uimage_inflate_reader(void* block, uint32_t block_size, void* ctx)
{
    struct uimage_inflatectx* c = ctx;
    ssize_t len = c->reader(block, block_size, c->rctx);
    if(len > 0) {
        len = MIN(c->remain, (size_t)len);
        c->remain -= len;
        c->dcrc = uimage_crc(c->dcrc, block, len);
    }

    return len;
}

static int uimage_decompress_gzip(const struct uimage_header* uh, int state_h,
                                  void* out, size_t* out_size,
                                  uimage_reader reader, void* rctx)
{
    size_t hbufsz = inflate_size + inflate_align - 1;
    void* hbuf = core_get_data(state_h);
    ALIGN_BUFFER(hbuf, hbufsz, inflate_align);

    struct uimage_inflatectx r_ctx;
    r_ctx.reader = reader;
    r_ctx.rctx = rctx;
    r_ctx.dcrc = 0;
    r_ctx.remain = uimage_get_size(uh);

    struct inflate_bufferctx w_ctx;
    w_ctx.buf = out;
    w_ctx.end = out + *out_size;

    int ret = inflate(hbuf, INFLATE_GZIP,
                      uimage_inflate_reader, &r_ctx,
                      inflate_buffer_writer, &w_ctx);
    if(ret)
        return ret;

    if(r_ctx.remain > 0)
        return -1;
    if(r_ctx.dcrc != uimage_get_dcrc(uh))
        return -2;

    *out_size = w_ctx.end - w_ctx.buf;
    return 0;
}
#endif /* HAVE_UIMAGE_COMP_GZIP */

static int uimage_decompress(const struct uimage_header* uh, int state_h,
                             void* out, size_t* out_size,
                             uimage_reader reader, void* rctx)
{
    size_t in_size = uimage_get_size(uh);
    ssize_t len;

    switch(uimage_get_comp(uh)) {
#ifdef HAVE_UIMAGE_COMP_NONE
    case IH_COMP_NONE:
        if(*out_size < in_size)
            return -2;

        len = reader(out, in_size, rctx);
        if(len < 0 || (size_t)len != in_size)
            return -3;

        if(uimage_crc(0, out, in_size) != uimage_get_dcrc(uh))
            return -4;

        *out_size = in_size;
        break;
#endif

#ifdef HAVE_UIMAGE_COMP_GZIP
    case IH_COMP_GZIP:
        return uimage_decompress_gzip(uh, state_h, out, out_size, reader, rctx);
#endif

    default:
        return -1;
    }

    return 0;
}

int uimage_load(struct uimage_header* uh, size_t* out_size,
                uimage_reader reader, void* rctx)
{
    if(reader(uh, sizeof(*uh), rctx) != (ssize_t)sizeof(*uh))
        return -1; /* read error */

    int ret = uimage_check_header(uh);
    if(ret)
        return ret;

    int state_h = uimage_alloc_state(uh);
    if(state_h < 0)
        return state_h;

    *out_size = 0;
    int out_h = core_alloc_maximum("uimage", out_size, &buflib_ops_locked);
    if(out_h <= 0) {
        ret = -1;
        goto err;
    }

    ret = uimage_decompress(uh, state_h, core_get_data(out_h), out_size,
                            reader, rctx);
    if(ret)
        goto err;

    core_shrink(out_h, core_get_data(out_h), *out_size);
    ret = 0;

  err:
    if(state_h > 0)
        core_free(state_h);
    if(out_h > 0) {
        if(ret == 0)
            ret = out_h;
        else
            core_free(out_h);
    }

    return ret;
}

ssize_t uimage_fd_reader(void* buf, size_t size, void* ctx)
{
    int fd = (intptr_t)ctx;
    return read(fd, buf, size);
}
