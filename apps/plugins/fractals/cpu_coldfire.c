/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Tomer Shalev
 *
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
#include "cpu_coldfire.h"

inline short muls16_asr10(short a, short b)
{
    asm (
        "muls.w  %[a],%[b]   \n"
        "asr.l   #8,%[b]     \n"
        "asr.l   #2,%[b]     \n"
        : /* outputs */
        [b]"+d"(b)
        : /* inputs */
        [a]"d" (a)
    );
    return b;
}

inline long muls32_asr26(long a, long b)
{
    long r, t1;
    asm (
        "mac.l   %[a], %[b], %%acc0  \n" /* multiply */
        "move.l  %%accext01, %[t1]   \n" /* get low part */
        "movclr.l %%acc0, %[r]       \n" /* get high part */
        "asl.l   #5, %[r]            \n" /* hi <<= 5, plus one free */
        "lsr.l   #3, %[t1]           \n" /* lo >>= 3 */
        "and.l   #0x1f, %[t1]        \n" /* mask out unrelated bits */
        "or.l    %[t1], %[r]         \n" /* combine result */
        : /* outputs */
        [r] "=d"(r),
        [t1]"=d"(t1)
        : /* inputs */
        [a] "d" (a),
        [b] "d" (b)
    );
    return r;
}

