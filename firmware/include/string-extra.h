/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Thomas Martitz
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
#ifndef STRING_EXTRA_H
#define STRING_EXTRA_H
#include <string.h>
#include "strlcpy.h"
#include "strlcat.h"
#include "strcasecmp.h"
#include "strcasestr.h"
#include "strmemccpy.h"
#include "strtok_r.h"
#include "memset16.h"

#if defined(WIN32) || defined(APPLICATION) \
        || defined(__PCTOOL__) || defined(SIMULATOR)
#ifndef mempcpy
#define mempcpy __builtin_mempcpy
#endif
#endif

/* copies a buffer of len bytes and null terminates it */
static inline char * strmemcpy(char *dst, const char *src, size_t len)
{
    /* NOTE: for now, assumes valid parameters! */
    *(char *)mempcpy(dst, src, len) = '\0';
    return dst;
}

/* duplicate and null-terminate a memory block on the stack with alloca() */
#define strmemdupa(s, l) \
    ({ const char *___s = (s);          \
       size_t ___l = (l);               \
       char *___buf = alloca(___l + 1); \
       strmemcpy(___buf, ___s, ___l); })

/* strdupa and strndupa may already be provided by a system's string.h */

#ifndef strdupa
/* duplicate an entire string on the stack with alloca() */
#define strdupa(s) \
    ({ const char *__s = (s); \
       strmemdupa((__s), strlen(__s)); })
#endif /* strdupa */

#ifndef strndupa
/* duplicate a string on the stack with alloca(), truncating it if it is too
   long */
#define strndupa(s, n) \
    ({ const char *__s = (s);     \
       size_t __n = (n);          \
       size_t __len = strlen(_s); \
       strmemdupa(__s, MIN(__n, __len)); })
#endif /* strndupa */

#endif /* STRING_EXTRA_H */
