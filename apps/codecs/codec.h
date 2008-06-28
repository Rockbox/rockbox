/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Jens Arnold
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

/* Global declarations to be used in rockbox software codecs */

#include "config.h"
#include "system.h"

#include <sys/types.h>

extern struct codec_api *ci;

/* Get these functions 'out of the way' of the standard functions. Not doing
 * so confuses the cygwin linker, and maybe others. These functions need to
 * be implemented elsewhere */
#define malloc(x) codec_malloc(x)
#define calloc(x,y) codec_calloc(x,y)
#define realloc(x,y) codec_realloc(x,y)
#define free(x) codec_free(x)
#define alloca(x) __builtin_alloca(x)

void* codec_malloc(size_t size);
void* codec_calloc(size_t nmemb, size_t size);
void* codec_realloc(void* ptr, size_t size);
void codec_free(void* ptr);

#define abs(x) ((x)>0?(x):-(x))
#define labs(x) abs(x)

void qsort(void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *));

