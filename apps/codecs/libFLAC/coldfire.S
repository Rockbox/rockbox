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
   FLAC__lpc_restore_signal programmed for MCF5249 or any other similar
   ColdFire V2 core with the EMAC unit.
 */
    .text
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
    jeq .exit         /* zero samples to process */
    moveq.l #8, %d3
    cmp.l %d3, %d2
    jgt .default      /* order is over 8, jump to default case */
    lea.l .jumptable, %a4
    move.l (%a4, %d2.l*4), %a4
    jmp (%a4)
    .align 4           /* avoid unaligned fetch */
.jumptable:
    .long .exit
    .long .order1
    .long .order2
    .long .order3
    .long .order4
    .long .order5
    .long .order6
    .long .order7
    .long .order8

.order8:
    movem.l (%a1), %d3-%d7/%a3-%a5 /* load lpc coefs */
    move.l (%a2)+, %a6             /* load first history sample */
.loop8:
    mac.l %a6, %a5, (%a2)+, %a6, %acc0
    mac.l %a6, %a4, (%a2)+, %a6, %acc0
    mac.l %a6, %a3, (%a2)+, %a6, %acc0
    mac.l %a6, %d7, (%a2)+, %a6, %acc0
    mac.l %a6, %d6, (%a2)+, %a6, %acc0
    mac.l %a6, %d5, (%a2)+, %a6, %acc0
    mac.l %a6, %d4, (%a2)+, %a6, %acc0
    mac.l %a6, %d3, (-7*4, %a2), %a6, %acc0 /* load for the next iteration */
    movclr.l %acc0, %d2    /* get sum */
    asr.l %d1, %d2         /* shift sum by lp_quantization bits */
    add.l (%a0)+, %d2      /* add residual and increment residual pointer */
    move.l %d2, (%a2)      /* save result to data */
    lea.l (-6*4, %a2), %a2 /* history pointer points at second element */
    subq.l #1, %d0         /* decrement counter */
    jne .loop8             /* are we done? */
    jra .exit

.order7:
    movem.l (%a1), %d3-%d7/%a3-%a4
    move.l (%a2)+, %a6
.loop7:
    mac.l %a6, %a4, (%a2)+, %a6, %acc0
    mac.l %a6, %a3, (%a2)+, %a6, %acc0
    mac.l %a6, %d7, (%a2)+, %a6, %acc0
    mac.l %a6, %d6, (%a2)+, %a6, %acc0
    mac.l %a6, %d5, (%a2)+, %a6, %acc0
    mac.l %a6, %d4, (%a2)+, %a6, %acc0
    mac.l %a6, %d3, (-6*4, %a2), %a6, %acc0
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l (%a0)+, %d2
    move.l %d2, (%a2)
    lea.l (-5*4, %a2), %a2
    subq.l #1, %d0
    jne .loop7
    jra .exit

.order6:
    movem.l (%a1), %d3-%d7/%a3
    move.l (%a2)+, %a6
.loop6:
    mac.l %a6, %a3, (%a2)+, %a6, %acc0
    mac.l %a6, %d7, (%a2)+, %a6, %acc0
    mac.l %a6, %d6, (%a2)+, %a6, %acc0
    mac.l %a6, %d5, (%a2)+, %a6, %acc0
    mac.l %a6, %d4, (%a2)+, %a6, %acc0
    mac.l %a6, %d3, (-5*4, %a2), %a6, %acc0
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l (%a0)+, %d2
    move.l %d2, (%a2)
    lea.l (-4*4, %a2), %a2
    subq.l #1, %d0
    jne .loop6
    jra .exit

.order5:
    movem.l (%a1), %d3-%d7
    move.l (%a2)+, %a6
.loop5:
    mac.l %a6, %d7, (%a2)+, %a6, %acc0
    mac.l %a6, %d6, (%a2)+, %a6, %acc0
    mac.l %a6, %d5, (%a2)+, %a6, %acc0
    mac.l %a6, %d4, (%a2)+, %a6, %acc0
    mac.l %a6, %d3, (-4*4, %a2), %a6, %acc0
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l (%a0)+, %d2
    move.l %d2, (%a2)
    lea.l (-3*4, %a2), %a2
    subq.l #1, %d0
    jne .loop5
    jra .exit

.order4:
    movem.l (%a1), %d3-%d6
    move.l (%a2)+, %a6
.loop4:
    mac.l %a6, %d6, (%a2)+, %a6, %acc0
    mac.l %a6, %d5, (%a2)+, %a6, %acc0
    mac.l %a6, %d4, (%a2)+, %a6, %acc0
    mac.l %a6, %d3, (-3*4, %a2), %a6, %acc0
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l (%a0)+, %d2
    move.l %d2, (%a2)
    subq.l #8, %a2
    subq.l #1, %d0
    jne .loop4
    jra .exit

.order3:
    movem.l (%a1), %d3-%d5
    move.l (%a2)+, %a6
.loop3:
    mac.l %a6, %d5, (%a2)+, %a6, %acc0
    mac.l %a6, %d4, (%a2)+, %a6, %acc0
    mac.l %a6, %d3, (-2*4, %a2), %a6, %acc0
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l (%a0)+, %d2
    move.l %d2, (%a2)
    subq.l #4, %a2
    subq.l #1, %d0
    jne .loop3
    jra .exit

.order2:
    movem.l (%a1), %d3-%d4
    move.l (%a2)+, %a6
.loop2:
    mac.l %a6, %d4, (%a2)+, %a6, %acc0
    mac.l %a6, %d3, %acc0 /* data for next iteration is already loaded */
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l (%a0)+, %d2
    move.l %d2, (%a2)
    subq.l #1, %d0
    jne .loop2
    jra .exit

.order1:
    /* no point in using mac here */
    move.l (%a1), %d3
.loop1:
    move.l %d3, %d2
    muls.l (%a2)+, %d2
    asr.l %d1, %d2
    add.l (%a0)+, %d2
    move.l %d2, (%a2)
    subq.l #1, %d0
    jne .loop1
    jra .exit
    
.default:
    /* we do the filtering in an unrolled by 4 loop as far as we can, and then
       do the rest in an ordinary one by one sample loop.
     */
    lea.l (%a1, %d2.l*4), %a3 /* need to start in the other end of coefs */
    move.l %a2, %a4     /* working copy of history pointer */
    move.l %d2, %d3
    lsr.l #2, %d3       /* coefs/4, number of iterations needed in next loop */
    move.l (%a4)+, %a6  /* preload lpc coef for loop */
.dloop1:
    lea.l (-16, %a3), %a3  /* move lpc coef pointer four samples backwards */
    movem.l (%a3), %d4-%d7 /* load four coefs */
    mac.l %a6, %d7, (%a4)+, %a6, %acc0
    mac.l %a6, %d6, (%a4)+, %a6, %acc0
    mac.l %a6, %d5, (%a4)+, %a6, %acc0
    mac.l %a6, %d4, (%a4)+, %a6, %acc0
    subq.l #1, %d3         /* any more unrolled loop operations left? */
    jne .dloop1
    
    move.l %d2, %d3
    moveq.l #3, %d4        /* mask 0x00000003 */
    and.l %d4, %d3         /* get the remaining samples to be filtered */
    jeq .dsave             /* no remaining samples */
.dloop2:
    move.l -(%a3), %d4     /* get lpc coef */
    mac.l %a6, %d4, (%a4)+, %a6, %acc0
    subq.l #1, %d3         /* any more iterations left? */
    jne .dloop2
.dsave:
    movclr.l %acc0, %d3    /* get result */
    asr.l %d1, %d3         /* shift lp_quantization bits right */
    add.l (%a0)+, %d3      /* add residual */
    move.l %d3, (-4, %a4)  /* history pointer is one sample past data pointer */
    addq.l #4, %a2         /* increment history pointer */
    subq.l #1, %d0         /* decrement data_len */
    jne .default           /* are we done? */
                           /* if so, fall through to exit */

.exit:
    movem.l (%sp), %d2-%d7/%a2-%a6
    lea.l (44, %sp), %sp
    rts
