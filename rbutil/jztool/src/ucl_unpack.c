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

#include "jztool.h"
#include "ucl/ucl.h"

static uint32_t xread32(const uint8_t* d)
{
    uint32_t r = 0;
    r |= d[0] << 24;
    r |= d[1] << 16;
    r |= d[2] <<  8;
    r |= d[3] <<  0;
    return r;
}

/* adapted from firmware/common/ucl_decompress.c */
jz_buffer* jz_ucl_unpack(const uint8_t* src, uint32_t src_len, uint32_t* dst_len)
{
    static const uint8_t magic[8] =
        {0x00, 0xe9, 0x55, 0x43, 0x4c, 0xff, 0x01, 0x1a};

    jz_buffer* buffer = NULL;

    /* make sure there are enough bytes for the header */
    if(src_len < 18)
        goto error;

    /* avoid memcmp for reasons of code size */
    for(size_t i = 0; i < sizeof(magic); ++i)
        if(src[i] != magic[i])
            goto error;

    /* read the other header fields */
    /* uint32_t flags = xread32(&src[8]); */
    uint8_t method = src[12];
    /* uint8_t level = src[13]; */
    uint32_t block_size = xread32(&src[14]);

    /* check supported compression method */
    if(method != 0x2e)
        goto error;

    /* validate */
    if(block_size < 1024 || block_size > 8*1024*1024)
        goto error;

    src += 18;
    src_len -= 18;

    /* Calculate amount of space that we might need & allocate a buffer:
     * - subtract 4 to account for end of file marker
     * - each block is block_size bytes + 8 bytes of header
     * - add one to nr_blocks to account for case where file size < block size
     * - total size =  max uncompressed size of block * nr_blocks
     */
    uint32_t nr_blocks = (src_len - 4) / (8 + block_size) + 1;
    uint32_t max_size = nr_blocks * (block_size + block_size/8 + 256);
    buffer = jz_buffer_alloc(max_size, NULL);
    if(!buffer)
        goto error;

    /* perform the decompression */
    uint32_t dst_ilen = buffer->size;
    uint8_t* dst = buffer->data;
    while(1) {
        if(src_len < 4)
            goto error;

        uint32_t out_len = xread32(src); src += 4, src_len -= 4;
        if(out_len == 0)
            break;

        if(src_len < 4)
            goto error;

        uint32_t in_len = xread32(src); src += 4, src_len -= 4;
        if(in_len > block_size || out_len > block_size ||
           in_len == 0 || in_len > out_len)
            goto error;

        if(src_len < in_len)
            goto error;

        if(in_len < out_len) {
            uint32_t actual_out_len = dst_ilen;
            int rc = ucl_nrv2e_decompress_safe_8(src, in_len, dst, &actual_out_len, NULL);
            if(rc != UCL_E_OK)
                goto error;
            if(actual_out_len != out_len)
                goto error;
        } else {
            for(size_t i = 0; i < in_len; ++i)
                dst[i] = src[i];
        }

        src += in_len;
        src_len -= in_len;
        dst += out_len;
        dst_ilen -= out_len;
    }

    /* subtract leftover number of bytes to get size of compressed output */
    *dst_len = buffer->size - dst_ilen;
    return buffer;

  error:
    jz_buffer_free(buffer);
    return NULL;
}
