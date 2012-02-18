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
    "1:                            \n"
        "eor   %3, %5, %1, asr #31 \n"
        "subs  %2, %2, #1          \n"
        "str   %3, [%0], #4        \n"
        "add   %1, %1, %4          \n"
        "bgt   1b                  \n"
        : "+r"(out), "+r"(*phase), "+r"(count),
          "=&r"(s)
        : "r"(step), "r"(amplitude));
}
