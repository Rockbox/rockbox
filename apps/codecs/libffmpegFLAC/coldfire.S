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
 
/* The following is an assembler optimised version of the LPC filtering
   routines needed for FLAC decoding. It is optimised for use with the
   MCF5249 processor, or any other similar ColdFire core with the EMAC unit.
   All LPC filtering up to order 10 is done in specially optimised unrolled
   loops, while every order above this is handled by a slower default routine.
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
.loop10:
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
    jne .loop10            | are we done?
    jra .exit

.order9:
    movem.l (%a1), %d4-%d7/%a1-%a5
    move.l (%a0)+, %a6
.loop9:
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
    jne .loop9
    jra .exit

.order8:
    movem.l (%a1), %d5-%d7/%a1-%a5
    move.l (%a0)+, %a6
.loop8:
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
    jne .loop8
    jra .exit

.order7:
    movem.l (%a1), %d6-%d7/%a1-%a5
    move.l (%a0)+, %a6
.loop7:
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
    jne .loop7
    jra .exit

.order6:
    movem.l (%a1), %d7/%a1-%a5
    move.l (%a0)+, %a6
.loop6:
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
    jne .loop6
    jra .exit

.order5:
    movem.l (%a1), %a1-%a5
    move.l (%a0)+, %a6
.loop5:
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
    jne .loop5
    jra .exit

.order4:
    movem.l (%a1), %a2-%a5
    move.l (%a0)+, %a6
.loop4:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, (%a0)+, %a6, %acc0
    mac.l %a6, %a3, (%a0)+, %a6, %acc0
    mac.l %a6, %a2, (-3*4, %a0), %a6, %acc0
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l %d2, (%a0)
    subq.l #8, %a0
    subq.l #1, %d0
    jne .loop4
    jra .exit

.order3:
    movem.l (%a1), %a3-%a5
    move.l (%a0)+, %a6
.loop3:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, (%a0)+, %a6, %acc0
    mac.l %a6, %a3, (-2*4, %a0), %a6, %acc0
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l %d2, (%a0)
    subq.l #4, %a0
    subq.l #1, %d0
    jne .loop3
    jra .exit

.order2:
    movem.l (%a1), %a4-%a5
    move.l (%a0)+, %a6
.loop2:
    mac.l %a6, %a5, (%a0)+, %a6, %acc0
    mac.l %a6, %a4, %acc0    | data for next iteration is already loaded
    movclr.l %acc0, %d2
    asr.l %d1, %d2
    add.l %d2, (%a0)
    subq.l #1, %d0
    jne .loop2
    jra .exit

.order1:
    | no point in using mac here
    move.l (%a1), %a5 
.loop1:
    move.l %a5, %d2
    muls.l (%a0)+, %d2
    asr.l %d1, %d2
    add.l %d2, (%a0)
    subq.l #1, %d0
    jne .loop1
    jra .exit
    
.default:
    /* we do the filtering in an unrolled by 4 loop as far as we can, and then
       do the rest by jump table. */
    lea.l (%a1, %d2.l*4), %a2 | need to start in the other end of coefs
    move.l %a0, %a3           | working copy of history pointer
    move.l %d2, %d3
    lsr.l #2, %d3             | coefs/4, num of iterations needed in next loop
    move.l (%a3)+, %a5        | preload data for loop
.dloop1:
    lea.l (-4*4, %a2), %a2    | move lpc coef pointer four samples backwards
    movem.l (%a2), %d4-%d7    | load four coefs
    mac.l %a5, %d7, (%a3)+, %a5, %acc0
    mac.l %a5, %d6, (%a3)+, %a5, %acc0
    mac.l %a5, %d5, (%a3)+, %a5, %acc0
    mac.l %a5, %d4, (%a3)+, %a5, %acc0
    subq.l #1, %d3            | any more unrolled loop operations left?
    jne .dloop1
    
    moveq.l #3, %d3           | mask 0x00000003
    and.l %d2, %d3            | get the remaining samples to be filtered
    jmp.l (2, %pc, %d3*2)     | then jump into mac.l chain
| jumptable:
    bra.b .dsave
    bra.b .oneleft
    bra.b .twoleft
| implicit .threeleft 
    move.l -(%a2), %d4
    mac.l %a5, %d4, (%a3)+, %a5, %acc0
.twoleft:
    move.l -(%a2), %d4
    mac.l %a5, %d4, (%a3)+, %a5, %acc0
.oneleft:
    move.l -(%a2), %d4
    mac.l %a5, %d4, (%a3)+, %a5, %acc0 | need this fetch to not break line below 
 
.dsave:
    subq.l #4, %a3            | we're one past the save location
    movclr.l %acc0, %d3       | get result
    asr.l %d1, %d3            | shift qlevel bits right
    add.l %d3, (%a3)          | add residual and save
    addq.l #4, %a0            | increment history pointer
    subq.l #1, %d0            | decrement sample count 
    jne .default              | are we done?
                              | if so, fall through to exit

.exit:
    movem.l (%sp), %d2-%d7/%a2-%a6
    lea.l (44, %sp), %sp
    rts
