/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2023 William Wilgus
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

#ifndef _CHUNKALLOC_H_
#define _CHUNKALLOC_H_
#include <stdbool.h>
#include <string.h>
#include "config.h"
#include "buflib.h"

#define CHUNK_ALLOC_INVALID ((size_t)-1)


struct chunk_alloc_header
{
    struct buflib_context *context; /* buflib context for all allocations */
    int chunk_handle; /* data handle of buflib allocated array of struct chunk */

    size_t chunk_bytes_total; /* total bytes in current chunk */
    size_t chunk_bytes_free;  /* free bytes in current chunk  */
    size_t chunk_size;        /* default chunk size    */
    size_t count;             /* total chunks possible */
    size_t current;           /* current chunk in use  */

    struct {
        int handle;
        size_t min_offset;
        size_t max_offset;
    } cached_chunk;
};

void chunk_alloc_free(struct chunk_alloc_header *hdr);

bool chunk_alloc_init(struct chunk_alloc_header *hdr,
                      struct buflib_context *ctx,
                      size_t chunk_size, size_t max_chunks);

bool chunk_realloc(struct chunk_alloc_header *hdr,
                   size_t chunk_size, size_t max_chunks);

void chunk_alloc_finalize(struct chunk_alloc_header *hdr);

size_t chunk_alloc(struct chunk_alloc_header *hdr, size_t size); /* Returns offset */

void* chunk_get_data(struct chunk_alloc_header *hdr, size_t offset); /* Returns data */

void chunk_put_data(struct chunk_alloc_header *hdr, void* data, size_t offset);

static inline bool chunk_alloc_is_initialized(struct chunk_alloc_header *hdr)
{
    return (hdr->chunk_handle > 0);
}
#endif
