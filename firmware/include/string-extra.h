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


#include <string.h>
#include "strlcpy.h"
#include "strlcat.h"
#include "strcasecmp.h"
#include "strcasestr.h"
#include "strtok_r.h"
#include "memset16.h"

/* duplicate a string on the stack with alloca() - copies l bytes + nul */
#define strlendupa(s, l) \
    ({ const char *__s = (s);                    \
       size_t __l = (l);                         \
       char *__buf = alloca(__l + 1);            \
       *(char *)mempcpy(__buf, __s, __l) = '\0'; \
       __buf; })

#ifndef strdupa
/* duplicate an entire string on the stack with alloca() */
#define strdupa(s) \
    ({ const char *_s = (s); \
       strlendupa((_s), strlen(_s)); })
#endif /* strdupa */

#ifndef strndupa
/* duplicate a string on the stack with alloca(), truncating it if it is too
   long */
#define strndupa(s, n) \
    ({ const char *_s = (s);     \
       size_t _n = (n);          \
       size_t _len = strlen(_s); \
       strlendupa(_s, MIN(_n, _len)); })
#endif /* strndupa */
