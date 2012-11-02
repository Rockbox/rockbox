/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (c) 2012 Marcin Bukat
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

#include "plugin.h"
#include <tlsf.h>

#undef  memset
#define memset(a,b,c)      rb->memset((a),(b),(c))
#undef  memmove
#define memmove(a,b,c)     rb->memmove((a),(b),(c))
#undef  memcmp
#define memcmp(a,b,c)      rb->memcmp((a),(b),(c))
#undef  strncmp
#define strncmp(a,b,c)     rb->strncmp((a),(b),(c))

#define fread(ptr, size, nmemb, stream) rb->read(stream, ptr, size*nmemb)
#define fclose(stream) rb->close(stream)
#define fdopen(a,b) ((a))

#define malloc(a) tlsf_malloc((a))
#define free(a)   tlsf_free((a))
#define realloc(a, b) tlsf_realloc((a),(b))
#define calloc(a,b) tlsf_calloc((a),(b))

#ifndef SIZE_MAX
#define SIZE_MAX INT_MAX
#endif
