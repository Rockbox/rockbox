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
#include "file.h"
#include "vuprintf.h"

static int fprfunc(void *pr, int letter)
{
    /* TODO: add a buffer to reduce write() calls */
    return write((intptr_t)pr, &(char){ letter }, 1) == 1 ? 1 : -1;
}

int fdprintf(int fd, const char *fmt, ...)
{
    int bytes;
    va_list ap;

    va_start(ap, fmt);
    bytes = vuprintf(fprfunc, (void *)(intptr_t)fd, fmt, ap);
    va_end(ap);

    return bytes;
}
