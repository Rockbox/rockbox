/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Nils Wallménius
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

/*
 * The following is an assembler optimised version of
 * void hybrid_filter(fltst *fs, int *in)
 */

#if defined(USE_IRAM)
    .section .icode
#else
    .text
#endif
    .align 2
    .global     hybrid_filter
    .type       hybrid_filter, @function

hybrid_filter:
    lea.l    (-8*4, %sp), %sp
    movem.l  %d2-%d7/%a2-%a3, (%sp)       | save some registers
    move.l   (8*4+4, %sp), %a0            | a0 = fs
    movem.l  (%a0), %d4-%d5               | d4 = fs->index, d5 = fs->error

    lea.l    (%a0, %d4.l*4), %a2
    lea.l    (148, %a2), %a1              | a1 = fs->dl + fs->index (*pA)
    lea.l    (52, %a2), %a2               | a2 = fs->dx + fs->index (*pM)

    move.l   (%a1)+, %a3                  | load one value from *pA (needed in every case)
    movem.l  (20, %a0), %d0-%d3           | load 4 values from *pB

    tst.l    %d5
    blt      .hf_negative
    bgt      .hf_positive

                                          | fs->error == 0
    mac.l    %d0, %a3, (%a1)+, %a3, %acc0
    mac.l    %d1, %a3, (%a1)+, %a3, %acc0
    mac.l    %d2, %a3, (%a1)+, %a3, %acc0
    mac.l    %d3, %a3, (%a1)+, %d4, %acc0
    movem.l  (4*4+20, %a0), %d0-%d3       | load 4 values from *pB
    bra      0f

    .hf_negative:                         | fs->error < 0
    movem.l  (%a2), %d4-%d7               | load 4 values from *pM
    sub.l    %d4, %d0
    sub.l    %d5, %d1
    sub.l    %d6, %d2
    sub.l    %d7, %d3
    movem.l  %d0-%d3, (20, %a0)
    mac.l    %d0, %a3, (%a1)+, %a3, %acc0
    mac.l    %d1, %a3, (%a1)+, %a3, %acc0
    mac.l    %d2, %a3, (%a1)+, %a3, %acc0
    mac.l    %d3, %a3, (%a1)+, %d4, %acc0

    movem.l  (4*4+20, %a0), %d0-%d3       | load 4 values from *pB
    movem.l  (4*4, %a2), %d5-%d7/%a3      | load 4 values from *pM
    sub.l    %d5, %d0
    sub.l    %d6, %d1
    sub.l    %d7, %d2
    sub.l    %a3, %d3
    movem.l  %d0-%d3, (4*4+20, %a0)
    bra      0f

    .hf_positive:                         | fs->error > 0
    movem.l  (%a2), %d4-%d7               | load 4 values from *pM
    add.l    %d4, %d0
    add.l    %d5, %d1
    add.l    %d6, %d2
    add.l    %d7, %d3
    movem.l  %d0-%d3, (20, %a0)
    mac.l    %d0, %a3, (%a1)+, %a3, %acc0
    mac.l    %d1, %a3, (%a1)+, %a3, %acc0
    mac.l    %d2, %a3, (%a1)+, %a3, %acc0
    mac.l    %d3, %a3, (%a1)+, %d4, %acc0

    movem.l  (4*4+20, %a0), %d0-%d3       | load 4 values from *pB
    movem.l  (4*4, %a2), %d5-%d7/%a3      | load 4 values from *pM
    add.l    %d5, %d0
    add.l    %d6, %d1
    add.l    %d7, %d2
    add.l    %a3, %d3
    movem.l  %d0-%d3, (4*4+20, %a0)

    0:

    mac.l    %d0, %d4, (%a1)+, %d5, %acc0 | common macro block
    mac.l    %d1, %d5, (%a1)+, %d6, %acc0
    mac.l    %d2, %d6, (%a1),  %d7, %acc0
    mac.l    %d3, %d7, %acc0

    move.l   (8*4+8, %sp), %a3            | a3 = in
    move.l   (%a3), %d3
    move.l   %d3, (4, %a0)                | fs->error = *in
    movclr.l %acc0, %d0                   | d0 = sum
    movem.l  (8,  %a0), %d1-%d2
    add.l    %d1, %d0                     | sum +=  fs->round
    asr.l    %d2, %d0                     | sum >>= fs->shift

    add.l    %d0, %d3
    move.l   %d3, (%a3)                   | *in += (sum >> fs->shift)

    move.l   %d3, ( 1*4, %a1)
    sub.l    %d7, %d3
    move.l   %d3, ( 0*4, %a1)
    sub.l    %d6, %d3
    move.l   %d3, (-1*4, %a1)
    sub.l    %d5, %d3
    move.l   %d3, (-2*4, %a1)

    moveq    #30,%d0
    asr.l    %d0,%d7
    asr.l    %d0,%d6
    asr.l    %d0,%d5
    asr.l    %d0,%d4

    moveq    #1,%d0
    or.l     %d0,%d7
    or.l     %d0,%d6
    or.l     %d0,%d5
    or.l     %d0,%d4

    lsl.l    #2,%d7
    lsl.l    #1,%d6
    lsl.l    #1,%d5
    movem.l  %d4-%d7, (8*4-3*4,%a2)       | store to *pM

    move.l   (%a0), %d0
    addq.l   #1, %d0
    cmp.l    #16, %d0                     | ++fs->index == 16 ?
    bne      1f

    movem.l  (16*4+148, %a0), %d0-%d7
    movem.l  %d0-%d7, (148, %a0)
    movem.l  (16*4+52, %a0), %d0-%d7
    movem.l  %d0-%d7, (52, %a0)
    clr.l    %d0                          | fs->index = 0
    1:

    move.l   %d0, (%a0)

    movem.l  (%sp), %d2-%d7/%a2-%a3       | restore stacked regs
    lea.l    (8*4, %sp), %sp
    rts

