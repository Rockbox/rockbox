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
#include <stdarg.h>
#include <string.h>
#include "file.h"
#include "format.h"

struct for_fprintf {
    int fd;    /* where to store it */
    int bytes; /* amount stored */
};

static int fprfunc(void *pr, int letter)
{
    struct for_fprintf *fpr  = (struct for_fprintf *)pr;
    int rc = write(fpr->fd, &letter, 1);

    if(rc > 0) {
        fpr->bytes++; /* count them */
        return 1;  /* we are ok */
    }

    return 0; /* failure */
}


int fdprintf(int fd, const char *fmt, ...)
{
    va_list ap;
    struct for_fprintf fpr;

    fpr.fd=fd;
    fpr.bytes=0;

    va_start(ap, fmt);
    vuprintf(fprfunc, &fpr, fmt, ap);
    va_end(ap);

    return fpr.bytes; /* return 0 on error */
}
