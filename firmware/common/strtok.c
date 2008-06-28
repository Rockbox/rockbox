/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied. *
 ****************************************************************************/

#include "config.h"

#ifndef HAVE_STRTOK_R
#include <stddef.h>
#include <string.h>

char *
strtok_r(char *ptr, const char *sep, char **end)
{
    if (!ptr)
        /* we got NULL input so then we get our last position instead */
        ptr = *end;

    /* pass all letters that are including in the separator string */
    while (*ptr && strchr(sep, *ptr))
        ++ptr;

    if (*ptr) {
        /* so this is where the next piece of string starts */
        char *start = ptr;

        /* set the end pointer to the first byte after the start */
        *end = start + 1;

        /* scan through the string to find where it ends, it ends on a
           null byte or a character that exists in the separator string */
        while (**end && !strchr(sep, **end))
            ++*end;

        if (**end) {
            /* the end is not a null byte */
            **end = '\0';  /* zero terminate it! */
            ++*end;        /* advance last pointer to beyond the null byte */
        }

        return start; /* return the position where the string starts */
    }

    /* we ended up on a null byte, there are no more strings to find! */
    return NULL;
}

#endif /* this was only compiled if strtok_r wasn't present */
