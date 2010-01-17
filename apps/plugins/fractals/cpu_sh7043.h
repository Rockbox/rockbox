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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _CPU_SH7043_H
#define _CPU_SH7043_H

inline static short muls16_asr10(short a, short b)
{
    short r;
    asm (
        "muls    %[a],%[b]   \n"
        "sts     macl,%[r]   \n"
        "shlr8   %[r]        \n"
        "shlr2   %[r]        \n"
        : /* outputs */
        [r]"=r"(r)
        : /* inputs */
        [a]"r"(a),
        [b]"r"(b)
    );
    return r;
}

inline static long muls32_asr26(long a, long b)
{
    long r, t1, t2, t3;
    asm (
        /* Signed 32bit * 32bit -> 64bit multiplication.
           Notation: xxab * xxcd, where each letter represents 16 bits.
           xx is the 64 bit sign extension.  */
        "swap.w  %[a],%[t1]  \n" /* t1 = ba */
        "mulu    %[t1],%[b]  \n" /* a * d */
        "swap.w  %[b],%[t3]  \n" /* t3 = dc */
        "sts     macl,%[t2]  \n" /* t2 = a * d */
        "mulu    %[t1],%[t3] \n" /* a * c */
        "sts     macl,%[r]   \n" /* hi = a * c */
        "mulu    %[a],%[t3]  \n" /* b * c */
        "clrt                \n"
        "sts     macl,%[t3]  \n" /* t3 = b * c */
        "addc    %[t2],%[t3] \n" /* t3 += t2, carry -> t2 */
        "movt    %[t2]       \n"
        "mulu    %[a],%[b]   \n" /* b * d */
        "mov     %[t3],%[t1] \n" /* t1t3 = t2t3 << 16 */
        "xtrct   %[t2],%[t1] \n"
        "shll16  %[t3]       \n"
        "sts     macl,%[t2]  \n" /* lo = b * d */
        "clrt                \n" /* hi.lo += t1t3 */
        "addc    %[t3],%[t2] \n"
        "addc    %[t1],%[r]  \n"
        "cmp/pz  %[a]        \n" /* ab >= 0 ? */
        "bt      1f          \n"
        "sub     %[b],%[r]   \n" /* no: hi -= cd (sign extension of ab is -1) */
    "1:                      \n"
        "cmp/pz  %[b]        \n" /* cd >= 0 ? */
        "bt      2f          \n"
        "sub     %[a],%[r]   \n" /* no: hi -= ab (sign extension of cd is -1) */
    "2:                      \n"
        /* Shift right by 26 and return low 32 bits */
        "shll2   %[r]        \n" /* hi <<= 6 */
        "shll2   %[r]        \n"
        "shll2   %[r]        \n"
        "shlr16  %[t2]       \n" /* (unsigned)lo >>= 26 */
        "shlr8   %[t2]       \n"
        "shlr2   %[t2]       \n"
        "or      %[t2],%[r]  \n" /* combine result */
        : /* outputs */
        [r] "=&r"(r),
        [t1]"=&r"(t1),
        [t2]"=&r"(t2),
        [t3]"=&r"(t3)
        : /* inputs */
        [a] "r"  (a),
        [b] "r"  (b)
    );
    return r;
}

#endif
