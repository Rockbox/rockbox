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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
/* The following is a first attempt at an assembler optimized version of
   FLAC__lpc_restore_signal programmed for MFC5249 or any other similar
   ColdFire V2 core with the EMAC unit.
*/
    .section .icode,"ax",@progbits
    .global FLAC__lpc_restore_signal_mcf5249
    .align 2
FLAC__lpc_restore_signal_mcf5249:
    lea.l (-44, %sp), %sp
    movem.l %d2-%d7/%a2-%a6, (%sp)
    move.l (44+4, %sp), %a0  /* residual */
    move.l (44+8, %sp), %d0  /* data_len */
    move.l (44+12, %sp), %a1 /* qlp_coef */
    move.l (44+16, %sp), %d2 /* order */
    move.l (44+20, %sp), %d1 /* lp_quantization */
    move.l (44+24, %sp), %a2 /* data */
    /* the data pointer always lags behind history pointer by 'order' samples.
       since we have one loop for each order, we can hard code this and free
       a register by not saving data pointer. 
     */
    move.l %d2, %d3 
    neg.l %d3 
    lea.l (%a2, %d3.l*4), %a2 /* history */
    clr.l %d3
    move.l %d3, %macsr /* we'll need integer mode for this */
    tst.l %d0          
    jeq .Lexit         /* zero samples to process */
    movq.l #8, %d3
    cmp.l %d3, %d2     /* coldfire v2 only has long cmp version */
    jgt .Ldefault      /* order is over 8, jump to default case */
    lea.l .Ljumptable, %a4
    move.l (%a4, %d2.l*4), %a4
    jmp (%a4)
    .align 4           /* avoid unaligned fetch */
.Ljumptable:
    .long .Lexit
    .long .Lorder1
    .long .Lorder2
    .long .Lorder3
    .long .Lorder4
    .long .Lorder5
    .long .Lorder6
    .long .Lorder7
    .long .Lorder8

.Lorder8:
    movem.l (%a1), %d3-%d7/%a3-%a5 /* load lpc coefs */
    movea.l (%a2), %a6             /* load first history sample */
.Lloop8:
    mac.l %a6, %a5, (1*4, %a2), %a6, %acc0
    mac.l %a6, %a4, (2*4, %a2), %a6, %acc0
    mac.l %a6, %a3, (3*4, %a2), %a6, %acc0
    mac.l %a6, %d7, (4*4, %a2), %a6, %acc0
    mac.l %a6, %d6, (5*4, %a2), %a6, %acc0
    mac.l %a6, %d5, (6*4, %a2), %a6, %acc0
    mac.l %a6, %d4, (7*4, %a2), %a6, %acc0
    mac.l %a6, %d3, (1*4, %a2), %a6, %acc0 /* load for the next iteration */
    addq.l #4, %a2        /* increment history pointer */
    movclr.l %acc0, %d2   /* get sum */
    asr.l %d1, %d2        /* shift sum by lp_quantization bits */
    add.l (%a0)+, %d2     /* add residual and increment residual pointer */
    move.l %d2, (28, %a2) /* save result to data */
    subq.l #1, %d0        /* decrement counter */
    jne .Lloop8           /* are we done? */
    jra .Lexit

.Lorder7:
    movem.l (%a1), %d3-%d7/%a3-%a4
    movea.l (%a2), %a6
.Lloop7:
    mac.l %a6, %a4, (1*4, %a2), %a6, %acc0
    mac.l %a6, %a3, (2*4, %a2), %a6, %acc0
    mac.l %a6, %d7, (3*4, %a2), %a6, %acc0
    mac.l %a6, %d6, (4*4, %a2), %a6, %acc0
    mac.l %a6, %d5, (5*4, %a2), %a6, %acc0
    mac.l %a6, %d4, (6*4, %a2), %a6, %acc0
    mac.l %a6, %d3, (1*4, %a2), %a6, %acc0
    addq.l #4, %a2
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l (%a0)+, %d2
    move.l %d2, (24, %a2)
    subq.l #1, %d0
    jne .Lloop7
    jra .Lexit

.Lorder6:
    movem.l (%a1), %d3-%d7/%a3
    movea.l (%a2), %a6
.Lloop6:
    mac.l %a6, %a3, (1*4, %a2), %a6, %acc0
    mac.l %a6, %d7, (2*4, %a2), %a6, %acc0
    mac.l %a6, %d6, (3*4, %a2), %a6, %acc0
    mac.l %a6, %d5, (4*4, %a2), %a6, %acc0
    mac.l %a6, %d4, (5*4, %a2), %a6, %acc0
    mac.l %a6, %d3, (1*4, %a2), %a6, %acc0
    addq.l #4, %a2
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l (%a0)+, %d2
    move.l %d2, (20, %a2)
    subq.l #1, %d0
    jne .Lloop6
    jra .Lexit

.Lorder5:
    movem.l (%a1), %d3-%d7
    movea.l (%a2), %a6
.Lloop5:
    mac.l %a6, %d7, (1*4, %a2), %a6, %acc0
    mac.l %a6, %d6, (2*4, %a2), %a6, %acc0
    mac.l %a6, %d5, (3*4, %a2), %a6, %acc0
    mac.l %a6, %d4, (4*4, %a2), %a6, %acc0
    mac.l %a6, %d3, (1*4, %a2), %a6, %acc0
    addq.l #4, %a2
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l (%a0)+, %d2
    move.l %d2, (16, %a2)
    subq.l #1, %d0
    jne .Lloop5
    jra .Lexit

.Lorder4:
    movem.l (%a1), %d3-%d6
    movea.l (%a2), %a6
.Lloop4:
    mac.l %a6, %d6, (1*4, %a2), %a6, %acc0
    mac.l %a6, %d5, (2*4, %a2), %a6, %acc0
    mac.l %a6, %d4, (3*4, %a2), %a6, %acc0
    mac.l %a6, %d3, (1*4, %a2), %a6, %acc0
    addq.l #4, %a2
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l (%a0)+, %d2
    move.l %d2, (12, %a2)
    subq.l #1, %d0
    jne .Lloop4
    jra .Lexit

.Lorder3:
    movem.l (%a1), %d3-%d5
    movea.l (%a2), %a6
.Lloop3:
    mac.l %a6, %d5, (1*4, %a2), %a6, %acc0
    mac.l %a6, %d4, (2*4, %a2), %a6, %acc0
    mac.l %a6, %d3, (1*4, %a2), %a6, %acc0
    addq.l #4, %a2
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l (%a0)+, %d2
    move.l %d2, (8, %a2)
    subq.l #1, %d0
    jne .Lloop3
    jra .Lexit

.Lorder2:
    movem.l (%a1), %d3-%d4
    movea.l (%a2), %a6
.Lloop2:
    mac.l %a6, %d4, (1*4, %a2), %a6, %acc0
    mac.l %a6, %d3, %acc0 /* data for next iteration is already loaded */
    addq.l #4, %a2
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l (%a0)+, %d2
    move.l %d2, (4, %a2)
    subq.l #1, %d0
    jne .Lloop2
    jra .Lexit

.Lorder1:
    /* no point in using mac here */
    move.l (%a1), %d3
.Lloop1:
    move.l %d3, %d2
    muls.l (%a2)+, %d2
    asr.l %d1, %d2
    add.l (%a0)+, %d2
    move.l %d2, (%a2)
    subq.l #1, %d0
    jne .Lloop1
    jra .Lexit
    
.Ldefault:
    /* we do the filtering in an unrolled by 4 loop as far as we can, and then
       do the rest in an ordinary on by one sample loop.
     */
    lea.l (%a1, %d2.l*4), %a3 /* need to start in the other end of coefs */
    movea.l %a2, %a4    /* working copy of history pointer */
    move.l %d2, %d3
    lsr.l #2, %d3       /* coefs/4, number of iterations needed in next loop */
    movea.l (%a4)+, %a6 /* preload lpc coef for loop */
.Ldloop1:
    lea.l (-16, %a3), %a3  /* move lpc coef pointer four samples backwards */
    movem.l (%a3), %d4-%d7 /* load four coefs */
    mac.l %a6, %d7, (%a4)+, %a6, %acc0
    mac.l %a6, %d6, (%a4)+, %a6, %acc0
    mac.l %a6, %d5, (%a4)+, %a6, %acc0
    mac.l %a6, %d4, (%a4)+, %a6, %acc0
    subq.l #1, %d3         /* any more unrolled loop operations left? */
    jne .Ldloop1
    
    move.l %d2, %d3
    movq.l #3, %d4        /* mask 0x00000003 */
    and.l %d4, %d3        /* get the remaining samples to be filtered */
    jeq .Ldsave           /* no remaining samples */
.Ldloop2:
    move.l -(%a3), %d4    /* get lpc coef */
    mac.l %a6, %d4, (%a4)+, %a6, %acc0
    subq.l #1, %d3        /* any more iterations left? */
    jne .Ldloop2
.Ldsave:
    movclr.l %acc0, %d3   /* get result */
    asr.l %d1, %d3        /* shift lp_quantization bits right */
    add.l (%a0)+, %d3     /* add residual */
    move.l %d3, (-4, %a4) /* history pointer is one sample past data pointer */
    addq.l #4, %a2        /* increment history pointer */
    subq.l #1, %d0        /* decrement data_len */
    jne .Ldefault         /* are we done? */
                          /* if so, fall through to exit */

.Lexit:
    movem.l (%sp), %d2-%d7/%a2-%a6
    lea.l (44, %sp), %sp
    rts
