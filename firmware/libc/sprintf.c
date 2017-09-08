/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Gary Czvitkovicz
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
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include "vuprintf.h"

/* ALSA library requires a more advanced snprintf, so let's not
   override it in simulator for Linux.  Note that Cygwin requires
   our snprintf or it produces garbled output after a while. */

struct for_snprintf {
    char *ptr;  /* where to store it */
    int  rem;   /* unwritten buffer remaining */
};

static int sprfunc(void *ptr, int letter)
{
    struct for_snprintf *pr = (struct for_snprintf *)ptr;

    if (pr->rem > 0) {
        if (--pr->rem == 0) {
            return 0;
        }

        *pr->ptr++ = letter;
        return 1;
    }
    else {
        if (pr->rem == -INT_MAX) {
            pr->rem = 1;
            return -1;
        }

        --pr->rem;
        return 1;
    }
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    if (size <= INT_MAX) {
        int bytes;
        struct for_snprintf pr;

        pr.ptr = buf;
        pr.rem = size;

        bytes = vuprintf(sprfunc, &pr, fmt, ap);

        if (size) {
            *pr.ptr = '\0';
        }
        else if (pr.rem > 0) {
            goto overflow;
        }

        return bytes;
    }

overflow:
    errno = EOVERFLOW;
    return -1;
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    if (size <= INT_MAX) {
        int bytes;
        struct for_snprintf pr;
        va_list ap;

        pr.ptr = buf;
        pr.rem = size;

        va_start(ap, fmt);
        bytes = vuprintf(sprfunc, &pr, fmt, ap);
        va_end(ap);

        if (size) {
            *pr.ptr = '\0';
        }
        else if (pr.rem > 0) {
            goto overflow;
        }

        return bytes;
    }

overflow:
    errno = EOVERFLOW;
    return -1;
}
