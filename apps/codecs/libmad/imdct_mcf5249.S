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
/* this will also be the home to III_imdct_l in the future */

    .global III_imdct_s
III_imdct_s:
    /* we need to save 9 registers and 36 samples of temp buffer */
    lea.l (-45*4, %sp), %sp
    movem.l %d2-%d7/%a2-%a4, (36*4, %sp)
    move.l (45*4 + 4, %sp), %a2  /* a2 = X */
    move.l %sp, %a3
  
    /* IMDCT */

    /* if additional precision is needed in this block, it is possible to
     * get more low bits out of the accext01 register _before_ doing the
     * movclrs.
     */
    sub.l %a0, %a0                      /* clear loop variable */
.imdctloop:                             /* outer loop label */
    lea.l imdct_s, %a1                  /* load pointer to imdct coefs in a1 */
    movem.l (%a2), %d0-%d5              /* load some input data in d0-d5 */
    lea.l (6*4, %a2), %a2

    clr.l %d7                           /* clear loop variable */
    move.l (%a1)+, %a4                  /* load imdct coef in a4 */
.macloop:                               /* inner loop label */
    mac.l %d0, %a4, (%a1)+, %a4, %acc0  /* mac sequence */
    mac.l %d1, %a4, (%a1)+, %a4, %acc0
    mac.l %d2, %a4, (%a1)+, %a4, %acc0
    mac.l %d3, %a4, (%a1)+, %a4, %acc0
    mac.l %d4, %a4, (%a1)+, %a4, %acc0
    mac.l %d5, %a4, (%a1)+, %a4, %acc0
    movclr.l %acc0, %d6                 /* get result, left shifted once */
    asl.l #3, %d6                       /* one shift free, shift three more */
    move.l %d6, (%a3, %d7.l*4)          /* yptr[i] = result */
    neg.l %d6
    neg.l %d7
    move.l %d6, (5*4, %a3, %d7.l*4)     /* yptr[5 - i] = -result */
    mac.l %d0, %a4, (%a1)+, %a4, %acc0  /* mac sequence */
    mac.l %d1, %a4, (%a1)+, %a4, %acc0
    mac.l %d2, %a4, (%a1)+, %a4, %acc0
    mac.l %d3, %a4, (%a1)+, %a4, %acc0
    mac.l %d4, %a4, (%a1)+, %a4, %acc0
    mac.l %d5, %a4, (%a1)+, %a4, %acc0
    movclr.l %acc0, %d6                 /* get result */
    asl.l #3, %d6
    move.l %d6, (11*4, %a3, %d7.l*4)    /* yptr[11 - i] = result */
    neg.l %d7
    move.l %d6, (6*4, %a3, %d7.l*4)     /* yptr[i + 6] = result */
    addq.l #1, %d7                      /* increment inner loop variable */
    moveq.l #3, %d6
    cmp.l %d6, %d7                      /* we do three inner loop iterations */
    jne .macloop

    lea.l (12*4, %a3), %a3              /* add pointer increment */
    addq.l #1, %a0                      /* increment outer loop variable */
    moveq.l #3, %d0
    cmp.l %d0, %a0                      /* we do three outer loop iterations */
    jne .imdctloop

    /* windowing, overlapping and concatenation */

    move.l (45*4 + 8, %sp), %a2       /* a2 = z */
    move.l %sp, %a3                   /* a3 = tmp buffer ptr */
    lea.l window_s, %a4               /* a4 = window coef pointer */

    moveq.l #6, %d7                   /* six iterations */
.overlaploop:
    clr.l (%a2)                       /* z[i + 0] = 0 */
    move.l (%a4), %d0
    move.l (%a3), %d2
    mac.l %d0, %d2, (6*4, %a4), %d1, %acc0
    move.l (6*4, %a3), %d2
    movclr.l %acc0, %d6
    asl.l #3, %d6
    move.l %d6, (6*4, %a2)            /* z[i + 6] = result */

    mac.l %d1, %d2, (12*4, %a3), %d2, %acc0
    mac.l %d0, %d2, (18*4, %a3), %d2, %acc0
    movclr.l %acc0, %d6
    asl.l #3, %d6
    move.l %d6, (12*4, %a2)           /* z[i + 12] = result */

    mac.l %d1, %d2, (24*4, %a3), %d2, %acc0
    mac.l %d0, %d2, (30*4, %a3), %d2, %acc0
    movclr.l %acc0, %d6
    asl.l #3, %d6
    move.l %d6, (18*4, %a2)           /* z[i + 18] = result */

    mac.l %d1, %d2, %acc0
    movclr.l %acc0, %d6
    asl.l #3, %d6
    move.l %d6, (24*4, %a2)           /* z[i + 24] = result */

    clr.l (30*4, %a2)                 /* z[i + 30] = 0 */
    addq.l #4, %a2                    /* increment all pointers */
    addq.l #4, %a3
    addq.l #4, %a4
    subq.l #1, %d7                    /* decrement loop counter */
    jne .overlaploop
    /* fall through to exit if we're done */
    
    /* clean up */
    movem.l (36*4, %sp), %d2-%d7/%a2-%a4
    lea.l (45*4, %sp), %sp
    rts

