/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

#define MEMPOOL_MAX 10
#include <inttypes.h>
#include "plugin.h"

int wpw_init_mempool(unsigned char mempool);
int wpw_init_mempool_pdm(unsigned char mempool,
                         unsigned char* mem,long memsize);

void wpw_reset_mempool(unsigned char mempool);
void wpw_destroy_mempool(unsigned char mempool);
void* wpw_malloc(unsigned char mempool,size_t size);
void* wpw_calloc(unsigned char mempool,size_t nmemb, size_t size);
void codec_free(unsigned char mempool,void* ptr);
void* wpw_realloc(unsigned char mempool,void* ptr, size_t size);
long wpw_available(unsigned char mempool);
