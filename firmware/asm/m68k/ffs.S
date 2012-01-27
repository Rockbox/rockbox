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
    .text
    .align  2
    .global find_first_set_bit
    .type   find_first_set_bit,@function
find_first_set_bit:
    | this is a coldfire version of the ffs algorithm devised by D.Seal
    | and posted to comp.sys.arm on 16 Feb 1994.
    |
    | Output modified to suit rockbox purposes.

    | Standard trick to isolate bottom bit in r0 or 0 if r0 = 0 on entry
    move.l  4(%sp), %d1          | %d1 = %d1 & -%d1
    lea.l   L_ffs_table, %a0     | %a0 = table address
    move.l  %d1, %d0             |
    neg.l   %d1                  |
    and.l   %d0, %d1             |

    | now %d1 has at most one set bit, call this X

    move.l  #0x0450fbaf, %d0     | %d0 = multiplier
    mulu.l  %d0, %d1             | %d1 = X * 0x0450fbaf

    | now lookup in table indexed on top 6 bits of %d0
    moveq.l #26, %d0             | %d0 = final shift count
    lsr.l   %d0, %d1             |

    | the top 24 bits of d0 are 0 so just load the value over it
    move.b  (%a0, %d1.l), %d0    |
    rts                          |
    .size   find_first_set_bit, .-find_first_set_bit

    .section .irodata
L_ffs_table:
    |        0   1   2   3   4   5   6   7
    |----------------------------------------------
    .byte   32,  0,  1, 12,  2,  6,  0, 13  |  0- 7
    .byte    3,  0,  7,  0,  0,  0,  0, 14  |  8-15
    .byte   10,  4,  0,  0,  8,  0,  0, 25  | 16-23
    .byte    0,  0,  0,  0,  0, 21, 27, 15  | 24-31
    .byte   31, 11,  5,  0,  0,  0,  0,  0  | 32-39
    .byte    9,  0,  0, 24,  0,  0, 20, 26  | 40-47
    .byte   30,  0,  0,  0,  0, 23,  0, 19  | 48-55
    .byte   29,  0, 22, 18, 28, 17, 16,  0  | 56-63
    .size    L_ffs_table, .-L_ffs_table
