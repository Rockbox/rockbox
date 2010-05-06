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

/*
 * Minimal printf and snprintf formatting functions
 *
 * These support %c %s %d and %x
 * Field width and zero-padding flag only
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <limits.h>
#include "format.h"

/* ALSA library requires a more advanced snprintf, so let's not
   override it in simulator for Linux.  Note that Cygwin requires
   our snprintf or it produces garbled output after a while. */

struct for_snprintf {
    unsigned char *ptr; /* where to store it */
    size_t bytes; /* amount already stored */
    size_t max;   /* max amount to store */
};

static int sprfunc(void *ptr, unsigned char letter)
{
    struct for_snprintf *pr = (struct for_snprintf *)ptr;
    if(pr->bytes < pr->max) {
        *pr->ptr = letter;
        pr->ptr++;
        pr->bytes++;
        return true;
    }
    return false; /* filled buffer */
}


int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    bool ok;
    va_list ap;
    struct for_snprintf pr;

    pr.ptr = (unsigned char *)buf;
    pr.bytes = 0;
    pr.max = size;

    va_start(ap, fmt);
    ok = format(sprfunc, &pr, fmt, ap);
    va_end(ap);

    /* make sure it ends with a trailing zero */
    pr.ptr[(pr.bytes < pr.max) ? 0 : -1] = '\0';
    
    return pr.bytes;
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    bool ok;
    struct for_snprintf pr;

    pr.ptr = (unsigned char *)buf;
    pr.bytes = 0;
    pr.max = size;

    ok = format(sprfunc, &pr, fmt, ap);

    /* make sure it ends with a trailing zero */
    pr.ptr[(pr.bytes < pr.max) ? 0 : -1] = '\0';
    
    return pr.bytes;
}
