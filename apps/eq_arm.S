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

    .text
    .global eq_filter
eq_filter:
    ldr r12, [sp]             @ get shift parameter
    stmdb sp!, { r0-r11, lr } @ save all params and clobbered regs 
    ldmia r1!, { r4-r8 }      @ load coefs
    mov r10, r1               @ loop prelude expects filter struct addr in r10

.filterloop:
    ldr r9, [sp]            @ get pointer to this channels data
    add r0, r9, #4
    str r0, [sp]            @ save back pointer to next channels data
    ldr r9, [r9]            @ r9 = x[]
    ldr r14, [sp, #8]       @ r14 = numsamples
    ldmia r10, { r0-r3 }    @ load history, r10 should be filter struct addr
    str r10, [sp, #4]       @ save it for loop end
.loop:
    /* r0-r3 = history, r4-r8 = coefs, r9 = x[], r10..r11 = accumulator,
       r12 = shift amount, r14 = number of samples.
       See eq_cf.S for explanation of what this loop does. Primary difference
       is the reordering of the equation we do here, which is done for register
       reuse reasons, we're pretty short on regs.
     */
    smull r10, r11, r6, r1  @ acc = b2*x[i - 2]
    mov r1, r0              @ fix input history
    smlal r10, r11, r5, r0  @ acc += b1*x[i - 1]
    ldr r0, [r9]            @ load input and fix history in same operation
    smlal r10, r11, r4, r0  @ acc += b0*x[i]
    smlal r10, r11, r7, r2  @ acc += a1*y[i - 1]
    smlal r10, r11, r8, r3  @ acc += a2*y[i - 2]
    mov r3, r2              @ fix output history
    mov r2, r11, lsl r12    @ get result
    @ TODO: arm makes it easy to mix in lower bits from r10 for extended
    @ precision here, but we don't have enough regs to save the shift factor
    @ we would need (32 - r12).
    str r2, [r9], #4        @ save result
    subs r14, r14, #1       @ are we done with this channel?
    bne .loop

    ldr r10, [sp, #4]       @ load filter struct pointer
    stmia r10!, { r0-r3 }   @ save back history
    ldr r11, [sp, #12]      @ load number of channels
    subs r11, r11, #1       @ all channels processed?
    strne r11, [sp, #12]
    bne .filterloop

    add sp, sp, #16         @ compensate for temp storage
    ldmia sp!, { r4-r11, pc }
