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
#include "cpu_arm.h"

inline long muls32_asr26(long a, long b)
{
    long r, t1;
    asm (
        "smull   %[r], %[t1], %[a], %[b]     \n"
        "mov     %[r], %[r], lsr #26         \n"
        "orr     %[r], %[r], %[t1], lsl #6   \n"
        : /* outputs */
        [r] "=&r,&r,&r"(r),
        [t1]"=&r,&r,&r"(t1)
        : /* inputs */
        [a] "%r,%r,%r" (a),
        [b] "r,0,1" (b)
    );
    return r;
}

