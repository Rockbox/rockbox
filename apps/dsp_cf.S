/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Thom Johansen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

    .section .idata,"aw",@progbits
crossfeed_coefs:    
    .long 0x4CCCCCCD    | LOW
    .long 0x33333333    | LOW_COMP
    .long -0x66666666   | HIGH_NEG
    .long 0x66666666    | HIGH_COMP
    .long 0x0CCCCCCD    | ATT
    .long 0x73333333    | ATT_COMP

    .section .text
    .global apply_crossfeed 
apply_crossfeed:
    lea.l (-44, %sp), %sp
    movem.l %d2-%d7/%a2-%a6, (%sp)      | save all regs
    move.l (44+4, %sp), %a4
    movem.l (%a4), %a4-%a5              | a4 = src[0], a5 = src[1]
    move.l (44+8, %sp), %d7             | d7 = count

    lea.l crossfeed_data, %a1
    lea.l crossfeed_coefs, %a6
    lea.l (16, %a1), %a0                | a0 = &delay[0][0]
    movem.l (%a1), %d0-%d3              | fetch filter history samples
    move.l (120, %a1), %d4              | fetch delay line index
    move.l (%a4), %d5                   | d5 = left sample
    move.l (%a5), %d6                   | d6 = right sample
    move.l (%a6)+, %a1                  | a1 = LOW value
    move.l (%a6)+, %a2                  | a2 = LOW_COMP value
    /* Register usage in loop:
     * a0 = &delay[0][0], a1 & a2 = coefs, a3 = temp storage,
     * a4 = src[0], a5 = src[1], a6 = &crossfeed_coefs[0],
     * d0 = low_left, d1 = low_right,
     * d2 = high_left, d3 = high_right,
     * d4 = delay line index,
     * d5 = src[0][i], d6 = src[1][i].
     * d7 = count
     */
.cfloop:
    | LOW*low_left + LOW_COMP*left
    mac.l %a1, %d0, %acc0 
    mac.l %a2, %d5, %acc0
    | LOW*low_right + LOW_COMP*right
    mac.l %a1, %d1, (%a6)+, %a1, %acc1  | a1 = HIGH_NEG
    mac.l %a2, %d6, (%a6)+, %a2, %acc1  | a2 = HIGH_COMP
    movclr.l %acc0, %d0                 | get low_left
    movclr.l %acc1, %d1                 | get low_right
    | HIGH_NEG*high_left + HIGH_COMP*left
    mac.l %a1, %d2, %acc0
    mac.l %a2, %d5, %acc0
    | HIGH_NEG*high_right + HIGH_COMP*right
    mac.l %a1, %d3, (%a6)+, %a1, %acc1  | a1 = ATT
    mac.l %a2, %d6, (%a6)+, %a2, %acc1  | a2 = ATT_COMP
    lea.l (-6*4, %a6), %a6              | coef = &coefs[0]
    move.l (%a0, %d4*4), %a3            | a3 = delay[0][idx]
    move.l (52, %a0, %d4*4), %d5        | d5 = delay[1][idx]
    movclr.l %acc0, %d2                 | get high_left
    movclr.l %acc1, %d3                 | get high_right
    | ATT*delay_r + ATT_COMP*high_left
    mac.l %a1, %d5, (4, %a4), %d5, %acc0 | d5 = src[0][i+1]
    mac.l %a2, %d2, (4, %a5), %d6, %acc0 | d6 = src[1][i+1]
    | ATT*delay_l + ATT_COMP*high_right
    mac.l %a1, %a3, (%a6)+, %a1, %acc1   | a1 = LOW
    mac.l %a2, %d3, (%a6)+, %a2, %acc1   | a2 = LOW_COMP
        
    | save crossfed samples to output
    movclr.l %acc0, %a3
    move.l %a3, (%a4)+                  | src[0][i++] = out_l
    movclr.l %acc1, %a3
    move.l %a3, (%a5)+                  | src[1][i++] = out_r
    move.l %d0, (%a0, %d4*4)            | delay[0][index] = low_left
    move.l %d1, (52, %a0, %d4*4)        | delay[1][index] = low_right */
    addq.l #1, %d4                      | index++ */
    cmp.l #13, %d4                      | if (index >= 13) {
    jlt .nowrap
    clr.l %d4                           |     index = 0
.nowrap:                                | }
    subq.l #1, %d7
    jne .cfloop
    | save data back to struct
    lea.l crossfeed_data, %a1
    movem.l %d0-%d3, (%a1)
    move.l %d4, (120, %a1)
    movem.l (%sp), %d2-%d7/%a2-%a6
    lea.l (44, %sp), %sp
    rts

