/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2011 Michael Sevakis
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

/* Actually output samples into beep_buf */
static FORCE_INLINE void beep_generate(uint32_t *out, int count,
                                       int32_t *phase, uint32_t step,
                                       uint32_t amplitude)
{
    uint32_t s;

    asm volatile (
    "1:                   \n"
        "move.l %1, %3    \n"
        "add.l  %4, %1    \n"
        "add.l  %3, %3    \n"
        "subx.l %3, %3    \n"
        "eor.l  %5, %3    \n"
        "move.l %3, (%0)+ \n"
        "subq.l #1, %2    \n"
        "bgt.b  1b        \n"
        : "+a"(out), "+d"(*phase), "+d"(count),
          "=&d"(s)
        : "r"(step), "d"(amplitude));
}
