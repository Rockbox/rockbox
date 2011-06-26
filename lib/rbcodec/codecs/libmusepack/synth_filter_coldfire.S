/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Thom Johansen 
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
 * static void 
 * mpc_decoder_windowing_D(MPC_SAMPLE_FORMAT * Data, 
 *                         const MPC_SAMPLE_FORMAT * V,
 *                         const MPC_SAMPLE_FORMAT * D)
 */

#if defined(USE_IRAM)
    .section .icode
#else
    .text
#endif
    .align 2
    .global     mpc_decoder_windowing_D
    .type       mpc_decoder_windowing_D, @function

mpc_decoder_windowing_D:
    lea.l    (-9*4, %sp), %sp
    movem.l  %d2-%d7/%a2-%a4, (%sp)       | save some registers
    movem.l  (9*4+4, %sp), %a0-%a2        | a0 = Data, a1 = V, a2 = D
    moveq.l  #32, %d0                     | loop counter

    move.l   (%a1), %a4
    0:                                    | loop
    movem.l  (%a2), %d1-%d7/%a3

    mac.l %d1, %a4, ( 96*4, %a1), %a4, %acc0
    mac.l %d2, %a4, (128*4, %a1), %a4, %acc0
    mac.l %d3, %a4, (224*4, %a1), %a4, %acc0
    mac.l %d4, %a4, (256*4, %a1), %a4, %acc0
    mac.l %d5, %a4, (352*4, %a1), %a4, %acc0
    mac.l %d6, %a4, (384*4, %a1), %a4, %acc0
    mac.l %d7, %a4, (480*4, %a1), %a4, %acc0
    mac.l %a3, %a4, (512*4, %a1), %a4, %acc0
    movem.l (8*4, %a2), %d1-%d7/%a3
    mac.l %d1, %a4, (608*4, %a1), %a4, %acc0
    mac.l %d2, %a4, (640*4, %a1), %a4, %acc0
    mac.l %d3, %a4, (736*4, %a1), %a4, %acc0
    mac.l %d4, %a4, (768*4, %a1), %a4, %acc0
    mac.l %d5, %a4, (864*4, %a1), %a4, %acc0
    mac.l %d6, %a4, (896*4, %a1), %a4, %acc0
    mac.l %d7, %a4, (992*4, %a1), %a4, %acc0
    mac.l %a3, %a4, (    4, %a1), %a4, %acc0

    lea.l    (16*4, %a2), %a2
    addq.l   #4, %a1
    movclr.l %acc0, %d1
    lsl.l    #1, %d1
    move.l   %d1, (%a0)+
    subq.l   #1, %d0
    bne 0b

    movem.l  (%sp), %d2-%d7/%a2-%a4       | restore stacked regs
    lea.l    (9*4, %sp), %sp
    rts

