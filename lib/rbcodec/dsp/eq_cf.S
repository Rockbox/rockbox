/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Thom Johansen
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

/* uncomment this to make filtering calculate lower bits after shifting.
 * without this, "shift" - 1 of the lower bits will be lost here.
 */
/* #define HIGH_PRECISION */

/*
 * void eq_filter(int32_t **x, struct eqfilter *f, unsigned num,
 *                unsigned channels, unsigned shift)
 */
    .text
    .global eq_filter
eq_filter:
    lea.l (-11*4, %sp), %sp 
    movem.l %d2-%d7/%a2-%a6, (%sp)    | save clobbered regs
    move.l (11*4+8, %sp), %a5         | fetch filter structure address
    move.l (11*4+20, %sp), %d7        | load shift count
    subq.l #1, %d7                    | EMAC gives us one free shift
#ifdef HIGH_PRECISION
    moveq.l #8, %d6
    sub.l %d7, %d6                    | shift for lower part of accumulator
#endif
    movem.l (%a5), %a0-%a4            | load coefs
    lea.l (5*4, %a5), %a5             | point to filter history

.filterloop:
    move.l (11*4+4, %sp), %a6         | load input channel pointer
    addq.l #4, (11*4+4, %sp)          | point x to next channel
    move.l (%a6), %a6
    move.l (11*4+12, %sp), %d5        | number of samples
    movem.l (%a5), %d0-%d3            | load filter history

    /* d0-d3 = history, d4 = temp, d5 = sample count, d6 = lower shift amount,
     * d7 = upper shift amount, a0-a4 = coefs, a5 = history pointer, a6 = x[]
     */
.loop:
    /* Direct form 1 filtering code. We assume DSP has put EMAC in frac mode.
     * y[n] = b0*x[i] + b1*x[i - 1] + b2*x[i - 2] + a1*y[i - 1] + a2*y[i - 2],
     * where y[] is output and x[] is input. This is performed out of order
     * to do parallel load of input value.
     */
    mac.l %a2, %d1, %acc0               | acc = b2*x[i - 2]
    move.l %d0, %d1                     | fix input history
    mac.l %a1, %d0, (%a6), %d0, %acc0   | acc += b1*x[i - 1], x[i] -> d0
    mac.l %a0, %d0, %acc0               | acc += b0*x[i]
    mac.l %a3, %d2, %acc0               | acc += a1*y[i - 1]
    mac.l %a4, %d3, %acc0               | acc += a2*y[i - 2]
    move.l %d2, %d3                     | fix output history
#ifdef HIGH_PRECISION
    move.l %accext01, %d2               | fetch lower part of accumulator
    move.b %d2, %d4                     | clear upper three bytes
    lsr.l %d6, %d4                      | shift lower bits
#endif
    movclr.l %acc0, %d2                 | fetch upper part of result
    asl.l %d7, %d2                      | restore fixed point format
#ifdef HIGH_PRECISION
    or.l %d2, %d4                       | combine lower and upper parts
#endif
    move.l %d2, (%a6)+                  | save result
    subq.l #1, %d5                      | are we done with this channel?
    jne .loop
    
    movem.l %d0-%d3, (%a5)              | save history back to struct
    lea.l (4*4, %a5), %a5               | point to next channel's history
    subq.l #1, (11*4+16, %sp)           | have we processed both channels?
    jne .filterloop

    movem.l (%sp), %d2-%d7/%a2-%a6
    lea.l (11*4, %sp), %sp
    rts

