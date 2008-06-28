/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Jens Arnold
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
#define LBLOCKSIZE (sizeof(long)/2)
#define UNALIGNED(X)   ((long)X & (sizeof(long) - 1))
#define TOO_SMALL(LEN) ((LEN) < LBLOCKSIZE)

void memset16(void *dst, int val, size_t len)
{
#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
    unsigned short *p = (unsigned short *)dst;

    while (len--)
        *p++ = val;
#else
    unsigned short *p = (unsigned short *)dst;
    unsigned int i;
    unsigned long buffer;
    unsigned long *aligned_addr;

    if (!TOO_SMALL(len) && !UNALIGNED(dst))
    {
        aligned_addr = (unsigned long *)dst;

        val &= 0xffff;
        if (LBLOCKSIZE == 2)
        {
            buffer = (val << 16) | val;
        }
        else
        {
            buffer = 0;
            for (i = 0; i < LBLOCKSIZE; i++)
	        buffer = (buffer << 16) | val;
        }

        while (len >= LBLOCKSIZE*4)
        {
            *aligned_addr++ = buffer;
            *aligned_addr++ = buffer;
            *aligned_addr++ = buffer;
            *aligned_addr++ = buffer;
            len -= 4*LBLOCKSIZE;
        }

        while (len >= LBLOCKSIZE)
        {
            *aligned_addr++ = buffer;
            len -= LBLOCKSIZE;
        }

        p = (unsigned short *)aligned_addr;
    }

    while (len--)
        *p++ = val;
#endif /* not PREFER_SIZE_OVER_SPEED */
}
