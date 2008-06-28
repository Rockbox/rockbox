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
 
/* The following are assembler optimised version of the LPC filtering
   routines needed for FLAC decoding. They is optimised for use with the
   MCF5249 processor, or any other similar ColdFire core with the EMAC unit.
 */

/* This routine deals with sample widths 16 and lower. All LPC filtering up to
   order 10 is done in specially optimised unrolled loops, while every order
   above this is handled by a slower default routine.
 */
    .section .icode,"ax",@progbits
    .global lpc_decode_emac
    .align 2
lpc_decode_emac:
    lea.l (-44, %sp), %sp
    movem.l %d2-%d7/%a2-%a6, (%sp)
    movem.l (44+4, %sp), %d0-%d2/%a0-%a1
    /* d0 = blocksize, d1 = qlevel, d2 = pred_order
       a0 = data, a1 = coeffs
     */
     
    /* the data pointer always lags behind history pointer by 'pred_order'
       samples. since we have one loop for each order, we can hard code this
       and free a register by not saving data pointer. 
     */ 
    move.l %d2, %d3 
    neg.l %d3
    lea.l (%a0, %d3.l*4), %a0 | history
    clr.l %d3
    move.l %d3, %macsr        | we'll need integer mode for this
    tst.l %d0          
    jeq .exit                 | zero samples to process, exit
    moveq.l #10, %d3
    cmp.l %d3, %d2
    jgt .default              | order is over 10, jump to default case
    jmp.l (2, %pc, %d2.l*4)   | jump to loop corresponding to pred_order
| jumptable:
    bra.w .exit               | zero order filter isn't possible, exit function
    bra.w .order1
    bra.w .order2
    bra.w .order3
    bra.w .order4
    bra.w .order5
    bra.w .order6
    bra.w .order7
    bra.w .order8
    bra.w .order9

| last jump table entry coincides with target, so leave it out
.order10:
    movem.l (%a1), %d3-%d7/%a1-%a5 | load lpc coefs
    move.l (%a0)+, %a6             | load first history sample
1:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, (%a0)+, %a6, %acc0
    mac.l %a6, %a3, (%a0)+, %a6, %acc0
    mac.l %a6, %a2, (%a0)+, %a6, %acc0
    mac.l %a6, %a1, (%a0)+, %a6, %acc0
    mac.l %a6, %d7, (%a0)+, %a6, %acc0
    mac.l %a6, %d6, (%a0)+, %a6, %acc0
    mac.l %a6, %d5, (%a0)+, %a6, %acc0
    mac.l %a6, %d4, (%a0)+, %a6, %acc0
    mac.l %a6, %d3, (-9*4, %a0), %a6, %acc0 | load for the next iteration
    movclr.l %acc0, %d2    | get sum
    asr.l %d1, %d2         | shift sum by qlevel bits
    add.l %d2, (%a0)       | add residual and save
    lea.l (-8*4, %a0), %a0 | point history back at second element
    subq.l #1, %d0         | decrement sample count
    jne 1b                 | are we done?
    jra .exit

.order9:
    movem.l (%a1), %d4-%d7/%a1-%a5
    move.l (%a0)+, %a6
1:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, (%a0)+, %a6, %acc0
    mac.l %a6, %a3, (%a0)+, %a6, %acc0
    mac.l %a6, %a2, (%a0)+, %a6, %acc0
    mac.l %a6, %a1, (%a0)+, %a6, %acc0
    mac.l %a6, %d7, (%a0)+, %a6, %acc0
    mac.l %a6, %d6, (%a0)+, %a6, %acc0
    mac.l %a6, %d5, (%a0)+, %a6, %acc0
    mac.l %a6, %d4, (-8*4, %a0), %a6, %acc0
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l %d2, (%a0)
    lea.l (-7*4, %a0), %a0
    subq.l #1, %d0
    jne 1b
    jra .exit

.order8:
    movem.l (%a1), %d5-%d7/%a1-%a5
    move.l (%a0)+, %a6
1:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, (%a0)+, %a6, %acc0
    mac.l %a6, %a3, (%a0)+, %a6, %acc0
    mac.l %a6, %a2, (%a0)+, %a6, %acc0
    mac.l %a6, %a1, (%a0)+, %a6, %acc0
    mac.l %a6, %d7, (%a0)+, %a6, %acc0
    mac.l %a6, %d6, (%a0)+, %a6, %acc0
    mac.l %a6, %d5, (-7*4, %a0), %a6, %acc0
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l %d2, (%a0)
    lea.l (-6*4, %a0), %a0
    subq.l #1, %d0
    jne 1b
    jra .exit

.order7:
    movem.l (%a1), %d6-%d7/%a1-%a5
    move.l (%a0)+, %a6
1:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, (%a0)+, %a6, %acc0
    mac.l %a6, %a3, (%a0)+, %a6, %acc0
    mac.l %a6, %a2, (%a0)+, %a6, %acc0
    mac.l %a6, %a1, (%a0)+, %a6, %acc0
    mac.l %a6, %d7, (%a0)+, %a6, %acc0
    mac.l %a6, %d6, (-6*4, %a0), %a6, %acc0
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l %d2, (%a0)
    lea.l (-5*4, %a0), %a0
    subq.l #1, %d0
    jne 1b
    jra .exit

.order6:
    movem.l (%a1), %d7/%a1-%a5
    move.l (%a0)+, %a6
1:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, (%a0)+, %a6, %acc0
    mac.l %a6, %a3, (%a0)+, %a6, %acc0
    mac.l %a6, %a2, (%a0)+, %a6, %acc0
    mac.l %a6, %a1, (%a0)+, %a6, %acc0
    mac.l %a6, %d7, (-5*4, %a0), %a6, %acc0
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l %d2, (%a0)
    lea.l (-4*4, %a0), %a0
    subq.l #1, %d0
    jne 1b
    jra .exit

.order5:
    movem.l (%a1), %a1-%a5
    move.l (%a0)+, %a6
1:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, (%a0)+, %a6, %acc0
    mac.l %a6, %a3, (%a0)+, %a6, %acc0
    mac.l %a6, %a2, (%a0)+, %a6, %acc0
    mac.l %a6, %a1, (-4*4, %a0), %a6, %acc0
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l %d2, (%a0)
    lea.l (-3*4, %a0), %a0
    subq.l #1, %d0
    jne 1b
    jra .exit

.order4:
    movem.l (%a1), %a2-%a5
    move.l (%a0)+, %a6
1:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, (%a0)+, %a6, %acc0
    mac.l %a6, %a3, (%a0)+, %a6, %acc0
    mac.l %a6, %a2, (-3*4, %a0), %a6, %acc0
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l %d2, (%a0)
    subq.l #8, %a0
    subq.l #1, %d0
    jne 1b
    jra .exit

.order3:
    movem.l (%a1), %a3-%a5
    move.l (%a0)+, %a6
1:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, (%a0)+, %a6, %acc0
    mac.l %a6, %a3, (-2*4, %a0), %a6, %acc0
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l %d2, (%a0)
    subq.l #4, %a0
    subq.l #1, %d0
    jne 1b
    jra .exit

.order2:
    movem.l (%a1), %a4-%a5
    move.l (%a0)+, %a6
1:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, %acc0    | data for next iteration is already loaded
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l %d2, (%a0)
    subq.l #1, %d0
    jne 1b
    jra .exit

.order1:
    | no point in using mac here
    move.l (%a1), %a5 
1:
    move.l %a5, %d2
    muls.l (%a0)+, %d2
    asr.l %d1, %d2
    add.l %d2, (%a0)
    subq.l #1, %d0
    jne 1b
    jra .exit
    
.default:
    /* we do the filtering in an unrolled by 4 loop as far as we can, and then
       do the rest by jump table. */
    lea.l (%a1, %d2.l*4), %a2 | need to start in the other end of coefs
    move.l %a0, %a3           | working copy of history pointer
    move.l %d2, %d3
    lsr.l #2, %d3             | coefs/4, num of iterations needed in next loop
    move.l (%a3)+, %a5        | preload data for loop
1:
    lea.l (-4*4, %a2), %a2    | move lpc coef pointer four samples backwards
    movem.l (%a2), %d4-%d7    | load four coefs
    mac.l %a5, %d7, (%a3)+, %a5, %acc0
    mac.l %a5, %d6, (%a3)+, %a5, %acc0
    mac.l %a5, %d5, (%a3)+, %a5, %acc0
    mac.l %a5, %d4, (%a3)+, %a5, %acc0
    subq.l #1, %d3            | any more unrolled loop operations left?
    jne 1b
    
    moveq.l #3, %d3           | mask 0x00000003
    and.l %d2, %d3            | get the remaining samples to be filtered
    jmp.l (2, %pc, %d3*2)     | then jump into mac.l chain
| jumptable:
    bra.b 3f                  | none left 
    bra.b 2f                  | one left
    bra.b 1f                  | two left
| three left
    move.l -(%a2), %d4
    mac.l %a5, %d4, (%a3)+, %a5, %acc0
1:
    move.l -(%a2), %d4
    mac.l %a5, %d4, (%a3)+, %a5, %acc0
2:
    move.l -(%a2), %d4
    mac.l %a5, %d4, (%a3)+, %a5, %acc0
3:
    movclr.l %acc0, %d3       | get result
    asr.l %d1, %d3            | shift qlevel bits right
    add.l %a5, %d3            | add residual, which is in a5 by now
    move.l %d3, -(%a3)        | save, a3 is also one past save location
    addq.l #4, %a0            | increment history pointer
    subq.l #1, %d0            | decrement sample count 
    jne .default              | are we done?
    jra .exit                 | if so, fall through to exit


/* This routine deals with sample widths 24 and lower. All LPC filtering up to
   order 8 is done in specially optimised unrolled loops, while every order
   above this is handled by a slower default routine.
 */
    .global lpc_decode_emac_wide
    .align 2
lpc_decode_emac_wide:
    lea.l (-44, %sp), %sp
    movem.l %d2-%d7/%a2-%a6, (%sp)
    movem.l (44+4, %sp), %d0-%d1/%d3/%a0-%a1
    /* d0 = blocksize, d1 = qlevel, d3 = pred_order
       a0 = data, a1 = coeffs
     */
     
    /* the data pointer always lags behind history pointer by 'pred_order'
       samples. since we have one loop for each order, we can hard code this
       and free a register by not saving data pointer. 
     */ 
    move.l %d3, %d2
    neg.l %d2
    lea.l (%a0, %d2.l*4), %a0 | history
    clr.l %d2
    move.l %d2, %macsr        | we'll need integer mode for this
    tst.l %d0          
    jeq .exit                 | zero samples to process, exit
    moveq.l #32, %d2
    sub.l %d1, %d2            | calculate shift amount for extension byte
    moveq.l #8, %d4
    cmp.l %d4, %d3
    jgt .wdefault             | order is over 8, jump to default case
    jmp.l (2, %pc, %d3.l*4)   | jump to loop corresponding to pred_order
| jumptable:
    bra.w .exit               | zero order filter isn't possible, exit function
    bra.w .worder1
    bra.w .worder2
    bra.w .worder3
    bra.w .worder4
    bra.w .worder5
    bra.w .worder6
    bra.w .worder7

| last jump table entry coincides with target, so leave it out
.worder8:
    movem.l (%a1), %d5-%d7/%a1-%a5 | load lpc coefs
    move.l (%a0)+, %a6             | load first history sample
1:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, (%a0)+, %a6, %acc0
    mac.l %a6, %a3, (%a0)+, %a6, %acc0
    mac.l %a6, %a2, (%a0)+, %a6, %acc0
    mac.l %a6, %a1, (%a0)+, %a6, %acc0
    mac.l %a6, %d7, (%a0)+, %a6, %acc0
    mac.l %a6, %d6, (%a0)+, %a6, %acc0
    mac.l %a6, %d5, (-7*4, %a0), %a6, %acc0 | load for the next iteration
    move.l %accext01, %d4  | get top 8 bits of sum
    movclr.l %acc0, %d3    | then botten 32 bits
    lsr.l %d1, %d3         | shift bottom bits qlevel bits right
    asl.l %d2, %d4         | shift top bits 32 - qlevel bits left
    or.l %d4, %d3          | now combine results
    add.l %d3, (%a0)       | add residual and save
    lea.l (-6*4, %a0), %a0 | point history back at second element
    subq.l #1, %d0         | decrement sample count
    jne 1b                 | are we done?
    jra .exit

.worder7:
    movem.l (%a1), %d6-%d7/%a1-%a5
    move.l (%a0)+, %a6
1:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, (%a0)+, %a6, %acc0
    mac.l %a6, %a3, (%a0)+, %a6, %acc0
    mac.l %a6, %a2, (%a0)+, %a6, %acc0
    mac.l %a6, %a1, (%a0)+, %a6, %acc0
    mac.l %a6, %d7, (%a0)+, %a6, %acc0
    mac.l %a6, %d6, (-6*4, %a0), %a6, %acc0
    move.l %accext01, %d4
    movclr.l %acc0, %d3
    lsr.l %d1, %d3
    asl.l %d2, %d4
    or.l %d4, %d3
    add.l %d3, (%a0)
    lea.l (-5*4, %a0), %a0
    subq.l #1, %d0
    jne 1b
    jra .exit

.worder6:
    movem.l (%a1), %d7/%a1-%a5
    move.l (%a0)+, %a6
1:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, (%a0)+, %a6, %acc0
    mac.l %a6, %a3, (%a0)+, %a6, %acc0
    mac.l %a6, %a2, (%a0)+, %a6, %acc0
    mac.l %a6, %a1, (%a0)+, %a6, %acc0
    mac.l %a6, %d7, (-5*4, %a0), %a6, %acc0
    move.l %accext01, %d4
    movclr.l %acc0, %d3
    lsr.l %d1, %d3
    asl.l %d2, %d4
    or.l %d4, %d3
    add.l %d3, (%a0)
    lea.l (-4*4, %a0), %a0
    subq.l #1, %d0
    jne 1b
    jra .exit

.worder5:
    movem.l (%a1), %a1-%a5
    move.l (%a0)+, %a6
1:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, (%a0)+, %a6, %acc0
    mac.l %a6, %a3, (%a0)+, %a6, %acc0
    mac.l %a6, %a2, (%a0)+, %a6, %acc0
    mac.l %a6, %a1, (-4*4, %a0), %a6, %acc0
    move.l %accext01, %d4
    movclr.l %acc0, %d3
    lsr.l %d1, %d3
    asl.l %d2, %d4
    or.l %d4, %d3
    add.l %d3, (%a0)
    lea.l (-3*4, %a0), %a0
    subq.l #1, %d0
    jne 1b
    jra .exit

.worder4:
    movem.l (%a1), %a2-%a5
    move.l (%a0)+, %a6
1:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, (%a0)+, %a6, %acc0
    mac.l %a6, %a3, (%a0)+, %a6, %acc0
    mac.l %a6, %a2, (-3*4, %a0), %a6, %acc0
    move.l %accext01, %d4
    movclr.l %acc0, %d3
    lsr.l %d1, %d3
    asl.l %d2, %d4
    or.l %d4, %d3
    add.l %d3, (%a0)
    subq.l #8, %a0
    subq.l #1, %d0
    jne 1b
    jra .exit

.worder3:
    movem.l (%a1), %a3-%a5
    move.l (%a0)+, %a6
1:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, (%a0)+, %a6, %acc0
    mac.l %a6, %a3, (-2*4, %a0), %a6, %acc0
    move.l %accext01, %d4
    movclr.l %acc0, %d3
    lsr.l %d1, %d3
    asl.l %d2, %d4
    or.l %d4, %d3
    add.l %d3, (%a0)
    subq.l #4, %a0
    subq.l #1, %d0
    jne 1b
    jra .exit

.worder2:
    movem.l (%a1), %a4-%a5
    move.l (%a0)+, %a6
1:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, %acc0    | data for next iteration is already loaded
    move.l %accext01, %d4
    movclr.l %acc0, %d3
    lsr.l %d1, %d3
    asl.l %d2, %d4
    or.l %d4, %d3
    add.l %d3, (%a0)
    subq.l #1, %d0
    jne 1b
    jra .exit

.worder1:
    move.l (%a1), %a5
    move.l (%a0)+, %a6
1:
    mac.l %a6, %a5, (%a0), %a6, %acc0
    move.l %accext01, %d4
    movclr.l %acc0, %d3
    lsr.l %d1, %d3
    asl.l %d2, %d4
    or.l %d4, %d3
    add.l %a6, %d3      | residual is already in a6
    move.l %d3, (%a0)+
    subq.l #1, %d0
    jne 1b
    jra .exit
    
.wdefault:
    /* we do the filtering in an unrolled by 4 loop as far as we can, and then
       do the rest by jump table. */
    lea.l (%a1, %d3.l*4), %a2 | need to start in the other end of coefs
    move.l %a0, %a3           | working copy of history pointer
    move.l %d3, %d4
    lsr.l #2, %d4             | coefs/4, num of iterations needed in next loop
    move.l (%a3)+, %a5        | preload data for loop
1:
    lea.l (-4*4, %a2), %a2     | move lpc coef pointer four samples backwards
    movem.l (%a2), %d5-%d7/%a4 | load four coefs
    mac.l %a5, %a4, (%a3)+, %a5, %acc0
    mac.l %a5, %d7, (%a3)+, %a5, %acc0
    mac.l %a5, %d6, (%a3)+, %a5, %acc0
    mac.l %a5, %d5, (%a3)+, %a5, %acc0
    subq.l #1, %d4            | any more unrolled loop operations left?
    jne 1b
    
    moveq.l #3, %d4           | mask 0x00000003
    and.l %d3, %d4            | get the remaining samples to be filtered
    jmp.l (2, %pc, %d4*2)     | then jump into mac.l chain
| jumptable:
    bra.b 3f                  | none left 
    bra.b 2f                  | one left
    bra.b 1f                  | two left
| three left
    move.l -(%a2), %d4
    mac.l %a5, %d4, (%a3)+, %a5, %acc0
1:
    move.l -(%a2), %d4
    mac.l %a5, %d4, (%a3)+, %a5, %acc0
2:
    move.l -(%a2), %d4
    mac.l %a5, %d4, (%a3)+, %a5, %acc0
3:
    move.l %accext01, %d5     | get high 32 bits of result
    movclr.l %acc0, %d4       | get low 32 bits of result
    lsr.l %d1, %d4            | shift qlevel bits right
    asl.l %d2, %d5            | shift 32 - qlevel bits left
    or.l %d5, %d4             | combine top and low bits after shift
    add.l %a5, %d4            | add residual, which is in a5 by now
    move.l %d4, -(%a3)        | save, a3 is also one past save location
    addq.l #4, %a0            | increment history pointer
    subq.l #1, %d0            | decrement sample count 
    jne .wdefault             | are we done?
                              | if so, fall through to exit

.exit:
    movem.l (%sp), %d2-%d7/%a2-%a6
    lea.l (44, %sp), %sp
    rts
