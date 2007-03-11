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

/****************************************************************************
 * void apply_crossfeed(int count, int32_t* src[])
 */
    .section .text
    .global apply_crossfeed 
apply_crossfeed:
    @ unfortunately, we ended up in a bit of a register squeeze here, and need
    @ to keep the count on the stack :/
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
    str     r0, [r12, #30*4]           @ save delay line index
    add     sp, sp, #8                 @ remove temp variables from stack
    ldmia   sp!, { r4-r11, pc }
.cfend:
    .size   apply_crossfeed,.cfend-apply_crossfeed

/****************************************************************************
 * int dsp_downsample(int count, struct dsp_data *data,
 *                    in32_t *src[], int32_t *dst[])
 */
    .section    .text
    .global     dsp_downsample
dsp_downsample:
    stmdb   sp!, { r4-r11, lr }     @ stack modified regs
    ldmib   r1, { r5-r6 }           @ r5 = num_channels,r6 = resample_data.delta
    sub     r5, r5, #1              @ pre-decrement num_channels for use
    add     r4, r1, #12             @ r4 = &resample_data.phase
    mov     r12, #0xff
    orr     r12, r12, #0xff00       @ r12 = 0xffff
.dschannel_loop:
    ldr     r1, [r4]                @ r1 = resample_data.phase
    ldr     r7, [r2, r5, lsl #2]    @ r7 = s = src[ch - 1]
    ldr     r8, [r3, r5, lsl #2]    @ r8 = d = dst[ch - 1]
    add     r9, r4, #4              @ r9 = &last_sample[0]
    ldr     r10, [r9, r5, lsl #2]   @ r10 = last_sample[ch - 1]
    sub     r11, r0, #1             
    ldr     r14, [r7, r11, lsl #2]  @ load last sample in s[] ...
    str     r14, [r9, r5, lsl #2]   @ and write as next frame's last_sample
    movs    r9, r1, lsr #16         @ r9 = pos = phase >> 16
    ldreq   r11, [r7]               @ if pos = 0, load src[0] and jump into loop
    beq     .dsuse_last_start
    cmp     r9, r0                  @ if pos >= count, we're already done
    bge     .dsloop_skip

    @ Register usage in loop:
    @ r0 = count, r1 = phase, r4 = &resample_data.phase, r5 = cur_channel,
    @ r6 = delta, r7 = s, r8 = d, r9 = pos, r10 = s[pos - 1], r11 = s[pos]
.dsloop:
    add     r9, r7, r9, lsl #2      @ r9 = &s[pos]
    ldmda   r9, { r10, r11 }        @ r10 = s[pos - 1], r11 = s[pos]
.dsuse_last_start:
    sub     r11, r11, r10           @ r11 = diff = s[pos] - s[pos - 1]
    @ keep frac in lower bits to take advantage of multiplier early termination
    and     r9, r1, r12             @ frac = phase & 0xffff
    smull   r9, r14, r11, r9
    add     r10, r10, r14, lsl #16
    add     r10, r10, r9, lsr #16   @ r10 = out = s[pos - 1] + frac*diff
    str     r10, [r8], #4           @ *d++ = out
    add     r1, r1, r6              @ phase += delta
    mov     r9, r1, lsr #16         @ pos = phase >> 16
    cmp     r9, r0                  @ pos < count?
    blt     .dsloop                 @ yup, do more samples
.dsloop_skip:
    subs    r5, r5, #1
    bpl     .dschannel_loop         @ if (--ch) >= 0, do another channel
    sub     r1, r1, r0, lsl #16     @ wrap phase back to start
    str     r1, [r4]                @ store back
    ldr     r1, [r3]                @ r1 = &dst[0]
    sub     r8, r8, r1              @ dst - &dst[0]
    mov     r0, r8, lsr #2          @ convert bytes->samples
    ldmia   sp!, { r4-r11, pc }     @ ... and we're out
.dsend:
    .size   dsp_downsample,.dsend-dsp_downsample

/****************************************************************************
 * int dsp_upsample(int count, struct dsp_data *dsp,
 *                  in32_t *src[], int32_t *dst[])
 */
    .section    .text
    .global     dsp_upsample
dsp_upsample:
    stmdb   sp!, { r4-r11, lr }     @ stack modified regs
    ldmib   r1, { r5-r6 }           @ r5 = num_channels,r6 = resample_data.delta
    sub     r5, r5, #1              @ pre-decrement num_channels for use
    add     r4, r1, #12             @ r4 = &resample_data.phase
    stmdb   sp!, { r0, r4 }         @ stack count and &resample_data.phase
.uschannel_loop:
    ldr     r12, [r4]               @ r12 = resample_data.phase
    mov     r1, r12, ror #16        @ swap halfword positions, we'll use carry
                                    @ to detect pos increments
    ldr     r7, [r2, r5, lsl #2]    @ r7 = s = src[ch - 1]
    ldr     r8, [r3, r5, lsl #2]    @ r8 = d = dst[ch - 1]
    add     r9, r4, #4              @ r9 = &last_sample[0]
    ldr     r10, [r9, r5, lsl #2]   @ r10 = last_sample[ch - 1]
    sub     r11, r0, #1             
    ldr     r14, [r7, r11, lsl #2]  @ load last sample in s[] ...
    str     r14, [r9, r5, lsl #2]   @ and write as next frame's last_sample
    add     r9, r7, r0, lsl #2      @ r9 = src_end = &src[count]
    movs    r14, r12, lsr #16       @ pos = resample_data.phase >> 16
    beq     .usstart_0              @ pos = 0
    cmp     r14, r0                 @ if pos >= count, we're already done
    bge     .usloop_skip
    add     r7, r7, r14, lsl #2     @ r7 = &s[pos]
    ldr     r10, [r7, #-4]          @ r11 = s[pos - 1]
    b       .usstart_0

    @ Register usage in loop:
    @ r0 = count, r1 = phase, r4 = &resample_data.phase, r5 = cur_channel,
    @ r6 = delta, r7 = s, r8 = d, r9 = src_end, r10 = s[pos - 1], r11 = s[pos]
.usloop_1:
    mov     r10, r11                @ r10 = previous sample
.usstart_0:
    ldr     r11, [r7], #4           @ r11 = next sample
    sub     r0, r11, r10            @ r0 = s[pos] - s[pos - 1]
.usloop_0:
    mov     r4, r1, lsr #16         @ r4 = frac = phase >> 16
    smull   r12, r14, r4, r0
    add     r14, r10, r14, lsl #16
    add     r14, r14, r12, lsr #16  @ r14 = out = s[pos - 1] + frac*diff
    str     r14, [r8], #4           @ *d++ = out
    adds    r1, r1, r6, lsl #16     @ phase += delta << 16
    bcc     .usloop_0               @ if carry is set, pos is incremented
    cmp     r7, r9                  @ if s < src_end, do another sample
    blo     .usloop_1
.usloop_skip:
    subs    r5, r5, #1
    ldmia   sp, { r0, r4 }          @ reload count and &resample_data.phase
    bpl     .uschannel_loop         @ if (--ch) >= 0, do another channel
    mov     r1, r1, ror #16         @ wrap phase back to start of next frame
    str     r1, [r4]                @ store back
    ldr     r1, [r3]                @ r1 = &dst[0]
    sub     r8, r8, r1              @ dst - &dst[0]
    mov     r0, r8, lsr #2          @ convert bytes->samples
    add     sp, sp, #8              @ adjust stack for temp variables
    ldmia   sp!, { r4-r11, pc }     @ ... and we're out
.usend:
    .size       dsp_upsample,.usend-dsp_upsample

