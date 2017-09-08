/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
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
#include <limits.h>
#include "file.h"
#include "vuprintf.h"

struct for_fprintf {
    int fd;  /* where to store it */
    int rem; /* amount remaining */
};

static int fprfunc(void *pr, int letter)
{
    struct for_fprintf *fpr  = (struct for_fprintf *)pr;

    /* TODO: add a small buffer to reduce write() calls */
    if (write(fpr->fd, &(char){ letter }, 1) > 0) {
        return --fpr->rem;
    }

    return -1;
}

int fdprintf(int fd, const char *fmt, ...)
{
    int bytes;
    struct for_fprintf fpr;
    va_list ap;

    fpr.fd  = fd;
    fpr.rem = INT_MAX;

    va_start(ap, fmt);
    bytes = vuprintf(fprfunc, &fpr, fmt, ap);
    va_end(ap);

    return bytes;
}
