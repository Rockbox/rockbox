/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Michael Sevakis
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
#include <time.h>
#include <errno.h>
#include "system.h"
#include "debug.h"

/* These functions we like but Windows doesn't because it implements the
   non-"_r" versions with thread-local storage in its multithreaded libs */

struct tm * localtime_r(const time_t *restrict timer,
                        struct tm *restrict result)
{
    if (!timer || !result)
    {
        errno = EINVAL;
        return NULL;
    }

    struct tm *tm = localtime(timer);
    if (!tm)
        return NULL;

    *result = *tm;
    return result;
}

struct tm * gmtime_r(const time_t *restrict timer,
                     struct tm *restrict result)
{
    if (!timer || !result)
    {
        errno = EINVAL;
        return NULL;
    }

    struct tm *tm = gmtime(timer);
    if (!tm)
        return NULL;

    *result = *tm;
    return result;
}
