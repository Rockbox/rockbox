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
#include "system.h"
#include "file.h"
#include "vuprintf.h"

#define FPR_WRBUF_CKSZ 32   /* write buffer chunk size */

struct for_fprintf {
    int fd;  /* where to store it */
    int rem; /* amount remaining */
    int idx; /* index of next buffer write */
    unsigned char wrbuf[FPR_WRBUF_CKSZ]; /* write buffer */
};

static int fpr_buffer_flush(struct for_fprintf *fpr)
{
    /* set idx to actual but negative unflushed count to signal error */
    ssize_t done = write(fpr->fd, fpr->wrbuf, fpr->idx);
    fpr->idx = MAX(done, 0) - fpr->idx;
    return fpr->idx;
}

static int fprfunc(void *pr, int letter)
{
    struct for_fprintf *fpr  = (struct for_fprintf *)pr;

    if (fpr->idx >= FPR_WRBUF_CKSZ && fpr_buffer_flush(fpr)) {
        return -1; /* don't count this one */
    }

    fpr->wrbuf[fpr->idx++] = letter;
    return --fpr->rem;
}

int fdprintf(int fd, const char *fmt, ...)
{
    int bytes;
    struct for_fprintf fpr;
    va_list ap;

    fpr.fd  = fd;
    fpr.rem = INT_MAX;
    fpr.idx = 0;

    va_start(ap, fmt);
    bytes = vuprintf(fprfunc, &fpr, fmt, ap);
    va_end(ap);

    /* flush any tail bytes */
    if (fpr.idx < 0 || fpr_buffer_flush(&fpr)) {
        bytes += fpr.idx; /* adjust for unflushed bytes */
    }

    return bytes;
}
