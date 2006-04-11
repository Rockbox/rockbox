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

    .section .text
    .global apply_crossfeed 
apply_crossfeed:
    lea.l (-44, %sp), %sp
    movem.l %d2-%d7/%a2-%a6, (%sp)      | save all regs
    move.l (44+4, %sp), %a4
    movem.l (%a4), %a4-%a5              | a4 = src[0], a5 = src[1]
    move.l (44+8, %sp), %d7             | d7 = count

    lea.l crossfeed_data, %a1
    lea.l (8*4, %a1), %a0               | a0 = &delay[0][0]
    move.l (%a1)+, %a6                  | a6 = direct gain
    movem.l (3*4, %a1), %d0-%d3         | fetch filter history samples
    move.l (33*4, %a1), %d4             | fetch delay line index
    movem.l (%a1), %a1-%a3              | load filter coefs
    move.l %d4, %d5
    lsl.l #3, %d5
    add.l %d5, %a0                      | point a0 to current delay position
|    lea.l (%d4*4, %a0), %a0
|    lea.l (%d4*4, %a0), %a0             | point a0 to current delay position
    /* Register usage in loop:
     * a0 = &delay[index][0], a1..a3 = b0, b1, a1 (filter coefs),
     * a4 = src[0], a5 = src[1], a6 = direct gain,
     * d0..d3 = history
     * d4 = delay line index,
     * d5,d6 = temp.
     * d7 = count
     */
.cfloop:
    mac.l %a2, %d0, (4, %a0), %d0, %acc0 | acc = b1*dr[n - 1] d0 = dr[n]
    mac.l %a1, %d0, %acc0               | acc += b0*dr[n]
    mac.l %a3, %d1, (%a4), %d5, %acc0   | acc += a1*y_l[n - 1], load left input
    move.l %acc0, %d1                   | get filtered delayed sample
    mac.l %a6, %d5, %acc0               | acc += gain*x_l[n]
    movclr.l %acc0, %d6
    move.l %d6, (%a4)+                  | write result

    mac.l %a2, %d2, (%a0), %d2, %acc0   | acc = b1*dl[n - 1], d2 = dl[n]
    move.l %d5, (%a0)+                  | save left input to delay line
    mac.l %a1, %d2, %acc0               | acc += b0*dl[n]
    mac.l %a3, %d3, (%a5), %d5, %acc0   | acc += a1*y_r[n - 1], load right input
    move.l %acc0, %d3                   | get filtered delayed sample
    mac.l %a6, %d5, %acc0               | acc += gain*x_r[n]
    move.l %d5, (%a0)+                  | save right input to delay line
    movclr.l %acc0, %d6
    move.l %d6, (%a5)+                  | write result

    addq.l #1, %d4                      | index++
    moveq.l #13, %d6
    cmp.l %d6, %d4                      | wrap index to 0 if it overflows
    jlt .nowrap
    moveq.l #13*8, %d4
    sub.l %d4, %a0                      | wrap back delay line ptr as well
    clr.l %d4
.nowrap:
    subq.l #1, %d7
    jne .cfloop
    | save data back to struct
    lea.l crossfeed_data + 4*4, %a1
    movem.l %d0-%d3, (%a1)
    move.l %d4, (30*4, %a1)
    movem.l (%sp), %d2-%d7/%a2-%a6
    lea.l (44, %sp), %sp
    rts

