/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 by James Buren (libflate adaptations for RockBox)
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

#ifndef _INFLATE_H_
#define _INFLATE_H_

#include <stdint.h>
#include <stddef.h>

enum {
    INFLATE_RAW,
    INFLATE_ZLIB,
    INFLATE_GZIP,
};

struct inflate;

typedef uint32_t (*inflate_reader) (void* block, uint32_t block_size, void* ctx);
typedef uint32_t (*inflate_writer) (const void* block, uint32_t block_size, void* ctx);

extern const uint32_t inflate_size;
extern const uint32_t inflate_align;

// 'it' must be allocated by the caller. it must be at least 'inflate_size' bytes
// and aligned at 'inflate_align' bytes. 'st' is the type of stream to decompress;
// see above enum for possible options.
int inflate(struct inflate* it, int st, inflate_reader read, void* rctx, inflate_writer write, void* wctx);

struct inflate_bufferctx {
    // initialize this with your input/output buffer.
    // the pointer is updated as data is read or written.
    void* buf;

    // buffer end marker (= buf + buf_size).
    void* end;
};

// reader and writer for using an in-memory buffer.
// Use 'inflate_bufferctx' as the context argument.
uint32_t inflate_buffer_reader(void* block, uint32_t block_size, void* ctx);
uint32_t inflate_buffer_writer(const void* block, uint32_t block_size, void* ctx);

// dummy writer used if you just want to figure out how big the decompressed
// data will be. It does not actually write any data. Example usage:
//
//   size_t size = 0;
//   inflate(it, st, read, rctx, inflate_getsize_writer, &size);
//
// Now 'size' will be the size of the decompressed data (assuming no errors).
uint32_t inflate_getsize_writer(const void* block, uint32_t block_size, void* ctx);

#endif
