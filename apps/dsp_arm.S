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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
 * void apply_crossfeed(int count, int32_t* src[])
 */
    .section .text
    .global apply_crossfeed 
apply_crossfeed:
    @ unfortunately, we ended up in a bit of a register squeeze here, and need
    @ to keep both the count and the delay line index on the stack :/
    stmdb   sp!, { r4-r11, lr }        @ stack modified regs
    ldmia   r1, { r2-r3 }              @ r2 = src[0], r3 = src[1]
    
    ldr     r1, =crossfeed_data
    ldmia   r1!, { r4-r11 }            @ load direct gain and filter data
    add     r12, r1, #13*4*2           @ calculate end of delay
    stmdb   sp!, { r0, r12 }           @ stack count and end of delay adr
    ldr     r0, [r1, #13*4*2]          @ fetch current delay line address

    /* Register usage in loop:
     * r0 = &delay[index][0], r1 = accumulator high, r2 = src[0], r3 = src[1],
     * r4 = direct gain, r5-r7 = b0, b1, a1 (filter coefs),
     * r8-r11 = filter history, r12 = temp, r14 = accumulator low
     */
.cfloop:
    smull   r14, r1, r6, r8            @ acc = b1*dr[n - 1]
    smlal   r14, r1, r7, r9            @ acc += a1*y_l[n - 1]
    ldr     r8, [r0, #4]               @ r8 = dr[n]
    smlal   r14, r1, r5, r8            @ acc += b0*dr[n]
    mov     r9, r1, lsl #1             @ fix format for filter history
    ldr     r12, [r2]                  @ load left input
    smlal   r14, r1, r4, r12           @ acc += gain*x_l[n] 
    mov     r1, r1, lsl #1             @ fix format
    str     r1, [r2], #4               @ save result

    smull   r14, r1, r6, r10           @ acc = b1*dl[n - 1]
    smlal   r14, r1, r7, r11           @ acc += a1*y_r[n - 1]
    ldr     r10, [r0]                  @ r10 = dl[n]
    str     r12, [r0], #4              @ save left input to delay line
    smlal   r14, r1, r5, r10           @ acc += b0*dl[n]
    mov     r11, r1, lsl #1            @ fix format for filter history
    ldr     r12, [r3]                  @ load right input
    smlal   r14, r1, r4, r12           @ acc += gain*x_r[n]
    str     r12, [r0], #4              @ save right input to delay line
    mov     r1, r1, lsl #1             @ fix format
    str     r1, [r3], #4               @ save result

    ldr     r12, [sp, #4]              @ fetch delay line end addr from stack
    cmp     r0, r12                    @ need to wrap to start of delay?
    subeq   r0, r0, #13*4*2            @ wrap back delay line ptr to start
 
    ldr     r1, [sp]                   @ fetch count from stack
    subs    r1, r1, #1                 @ are we finished?
    strne   r1, [sp]                   @ nope, save count back to stack
    bne     .cfloop
    
    @ save data back to struct
    ldr     r12, =crossfeed_data + 4*4
    stmia   r12, { r8-r11 }            @ save filter history
    str     r0, [r12, #30*4]            @ save delay line index
    add     sp, sp, #8                 @ remove temp variables from stack
    ldmia   sp!, { r4-r11, pc }

