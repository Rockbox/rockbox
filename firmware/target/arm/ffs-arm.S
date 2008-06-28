/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Michael Sevakis
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
 #include "config.h"

/****************************************************************************
 * int find_first_set_bit(uint32_t val);
 *
 * Find the index of the least significant set bit in the 32-bit word.
 *
 * return values:
 *   0  - bit 0 is set
 *   1  - bit 1 is set
 *   ...
 *   31 - bit 31 is set
 *   32 - no bits set
 ****************************************************************************/
    .align  2
    .global find_first_set_bit
    .type   find_first_set_bit,%function
find_first_set_bit:
    @ Standard trick to isolate bottom bit in r0 or 0 if r0 = 0 on entry
    rsb     r2, r0, #0               @ r1 = r0 & -r0
    ands    r1, r0, r2               @
    
    @ now r1 has at most one set bit, call this X
    
#if ARM_ARCH >= 5
    clz     r0, r1                   @ Get lead 0's count
    rsbne   r0, r0, #31              @ lead 0's -> bit index
    bx      lr                       @
#else
    @ this is the ffs algorithm devised by D.Seal and posted to
    @ comp.sys.arm on 16 Feb 1994.
    @
    @ Output modified to suit Rockbox purposes.

    adr     r2, L_ffs_table
    orrne   r1, r1, r1, lsl #4       @ r1 = X * 0x11
    orrne   r1, r1, r1, lsl #6       @ r1 = X * 0x451
    rsbne   r1, r1, r1, lsl #16      @ r1 = X * 0x0450fbaf

    @ now lookup in table indexed on top 6 bits of r1
    ldrb    r0, [ r2, r1, lsr #26 ]  @
    bx      lr                       @

L_ffs_table:
    @        0   1   2   3   4   5   6   7
    @----------------------------------------------
    .byte   32,  0,  1, 12,  2,  6,  0, 13  @  0- 7
    .byte    3,  0,  7,  0,  0,  0,  0, 14  @  8-15
    .byte   10,  4,  0,  0,  8,  0,  0, 25  @ 16-23
    .byte    0,  0,  0,  0,  0, 21, 27, 15  @ 24-31
    .byte   31, 11,  5,  0,  0,  0,  0,  0  @ 32-39
    .byte    9,  0,  0, 24,  0,  0, 20, 26  @ 40-47
    .byte   30,  0,  0,  0,  0, 23,  0, 19  @ 48-55
    .byte   29,  0, 22, 18, 28, 17, 16,  0  @ 56-63
#endif
    .size   find_first_set_bit, .-find_first_set_bit
