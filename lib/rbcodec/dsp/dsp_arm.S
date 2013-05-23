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
 * Copyright (C) 2010 Bertrik Sikken
 * Copyright (C) 2012 Michael Sevakis
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
 #include "rbcodecconfig.h"

/****************************************************************************
 *  void channel_mode_proc_mono(struct dsp_proc_entry *this,
 *                              struct dsp_buffer **buf_p)
 */
    .section    .text, "ax",%progbits
    .global channel_mode_proc_mono
    .type   channel_mode_proc_mono, %function
channel_mode_proc_mono:
    @ input: r0 = this, r1 = buf_p
    ldr     r1, [r1]                   @ r1 = buf = *buf_p;
    stmfd   sp!, { r4, lr }            @
                                       @
    ldmia   r1, { r0-r2 }              @ r0 = buf->remcount, r1 = buf->p32[0],
                                       @ r2 = buf->p32[1]
    subs    r0, r0, #1                 @ odd: end at 0; even: end at -1
    beq     .mono_singlesample         @ Zero? Only one sample!
                                       @
.monoloop:                             @
    ldmia   r1, { r3, r4 }             @ r3, r4 = Li0, Li1
    ldmia   r2, { r12, r14 }           @ r12, r14 = Ri0, Ri1
    mov     r3, r3, asr #1             @ Mo0 = Li0 / 2 + Ri0 / 2
    mov     r4, r4, asr #1             @ Mo1 = Li1 / 2 + Ri1 / 2
    add     r12, r3, r12, asr #1       @
    add     r14, r4, r14, asr #1       @
    subs    r0, r0, #2                 @
    stmia   r1!, { r12, r14 }          @ store Mo0, Mo1
    stmia   r2!, { r12, r14 }          @ store Mo0, Mo1
    bgt     .monoloop                  @
                                       @
    ldmpc   cond=lt, regs=r4           @ if count was even, we're done
                                       @
.mono_singlesample:                    @
    ldr     r3, [r1]                   @ r3 = Ls
    ldr     r12, [r2]                  @ r12 = Rs
    mov     r3, r3, asr #1             @ Mo = Ls / 2 + Rs / 2
    add     r12, r3, r12, asr #1       @
    str     r12, [r1]                  @ store Mo
    str     r12, [r2]                  @ store Mo
                                       @
    ldmpc   regs=r4                    @
    .size   channel_mode_proc_mono, .-channel_mode_proc_mono

/****************************************************************************
 * void channel_mode_proc_custom(struct dsp_proc_entry *this,
 *                               struct dsp_buffer **buf_p)
 */
    .section    .text, "ax",%progbits
    .global channel_mode_proc_custom
    .type   channel_mode_proc_custom, %function
channel_mode_proc_custom:
    @ input: r0 = this, r1 = buf_p
    ldr     r2, [r0]                   @ r2 = &channel_mode_data = this->data
    ldr     r1, [r1]                   @ r1 = buf = *buf_p;

    stmfd   sp!, { r4-r10, lr }

    ldmia   r2, { r3, r4 }             @ r3 = sw_gain, r4 = sw_cross

    ldmia   r1, { r0-r2 }              @ r0 = buf->remcount, r1 = buf->p32[0],
                                       @ r2 = buf->p32[1]

    subs    r0, r0, #1
    beq     .custom_single_sample      @ Zero? Only one sample!

.custom_loop:
    ldmia   r1, { r5, r6 }             @ r5 = Li0, r6 = Li1
    ldmia   r2, { r7, r8 }             @ r7 = Ri0, r8 = Ri1

    subs    r0, r0, #2

    smull   r9, r10, r5, r3            @ Lc0 = Li0*gain
    smull   r12, r14, r7, r3           @ Rc0 = Ri0*gain
    smlal   r9, r10, r7, r4            @ Lc0 += Ri0*cross
    smlal   r12, r14, r5, r4           @ Rc0 += Li0*cross

    mov     r9, r9, lsr #31            @ Convert to s0.31
    mov     r12, r12, lsr #31
    orr     r5, r9, r10, asl #1
    orr     r7, r12, r14, asl #1

    smull   r9, r10, r6, r3            @ Lc1 = Li1*gain
    smull   r12, r14, r8, r3           @ Rc1 = Ri1*gain
    smlal   r9, r10, r8, r4            @ Lc1 += Ri1*cross
    smlal   r12, r14, r6, r4           @ Rc1 += Li1*cross

    mov     r9, r9, lsr #31            @ Convert to s0.31
    mov     r12, r12, lsr #31
    orr     r6, r9, r10, asl #1
    orr     r8, r12, r14, asl #1

    stmia   r1!, { r5, r6 }            @ Store Lc0, Lc1
    stmia   r2!, { r7, r8 }            @ Store Rc0, Rc1

    bgt     .custom_loop

    ldmpc   cond=lt, regs=r4-r10       @ < 0? even count

.custom_single_sample:
    ldr     r5, [r1]                   @ handle odd sample
    ldr     r7, [r2]

    smull   r9, r10, r5, r3            @ Lc0 = Li0*gain
    smull   r12, r14, r7, r3           @ Rc0 = Ri0*gain
    smlal   r9, r10, r7, r4            @ Lc0 += Ri0*cross
    smlal   r12, r14, r5, r4           @ Rc0 += Li0*cross

    mov     r9, r9, lsr #31            @ Convert to s0.31
    mov     r12, r12, lsr #31
    orr     r5, r9, r10, asl #1
    orr     r7, r12, r14, asl #1

    str     r5, [r1]                   @ Store Lc0
    str     r7, [r2]                   @ Store Rc0

    ldmpc   regs=r4-r10
    .size   channel_mode_proc_custom, .-channel_mode_proc_custom

/****************************************************************************
 *  void channel_mode_proc_karaoke(struct dsp_proc_entry *this,
 *                                 struct dsp_buffer **buf_p)
 */
    .section    .text, "ax",%progbits
    .global channel_mode_proc_karaoke
    .type   channel_mode_proc_karaoke, %function
channel_mode_proc_karaoke:
    @ input: r0 = this, r1 = buf_p
    ldr     r1, [r1]                   @ r1 = buf = *buf_p;
    stmfd   sp!, { r4, lr }            @
                                       @
    ldmia   r1, { r0-r2 }              @ r0 = buf->remcount, r1 = buf->p32[0],
                                       @ r2 = buf->p32[1]
    subs    r0, r0, #1                 @ odd: end at 0; even: end at -1
    beq     .karaoke_singlesample      @ Zero? Only one sample!
                                       @
.karaokeloop:                          @
    ldmia   r1, { r3, r4 }             @ r3, r4  = Li0, Li1
    ldmia   r2, { r12, r14 }           @ r12, r14 = Ri0, Ri1
    mov     r3, r3, asr #1             @ Lo0 = Li0 / 2 - Ri0 / 2
    mov     r4, r4, asr #1             @ Lo1 = Li1 / 2 - Ri1 / 2
    sub     r3, r3, r12, asr #1        @
    sub     r4, r4, r14, asr #1        @
    rsb     r12, r3, #0                @ Ro0 = -Lk0 = Rs0 / 2 - Ls0 / 2
    rsb     r14, r4, #0                @ Ro1 = -Lk1 = Ri1 / 2 - Li1 / 2
    subs    r0, r0, #2                 @
    stmia   r1!, { r3, r4 }            @ store Lo0, Lo1
    stmia   r2!, { r12, r14 }          @ store Ro0, Ro1
    bgt     .karaokeloop               @
                                       @
    ldmpc   cond=lt, regs=r4           @ if count was even, we're done
                                       @
.karaoke_singlesample:                 @
    ldr     r3, [r1]                   @ r3 = Li
    ldr     r12, [r2]                  @ r12 = Ri
    mov     r3, r3, asr #1             @ Lk = Li / 2 - Ri /2
    sub     r3, r3, r12, asr #1        @
    rsb     r12, r3, #0                @ Rk = -Lo = Ri / 2 - Li / 2
    str     r3, [r1]                   @ store Lo
    str     r12, [r2]                  @ store Ro
                                       @
    ldmpc   regs=r4                    @
    .size   channel_mode_proc_karaoke, .-channel_mode_proc_karaoke

/****************************************************************************
 * void crossfeed_process(struct dsp_proc_entry *this,
 *                        struct dsp_buffer **buf_p)
 */
    .section    .text, "ax",%progbits
    .global crossfeed_process
crossfeed_process:
    @ input: r0 = this, r1 = buf_p
    @ unfortunately, we ended up in a bit of a register squeeze here, and need
    @ to keep the count on the stack :/
    ldr     r1, [r1]                   @ r1 = buf = *buf_p;
    stmfd   sp!, { r4-r11, lr }        @ stack modified regs
    ldr     r0, [r0]                   @ r0 = this->data = &crossfeed_state
    ldmia   r1, { r1-r3 }              @ r1 = buf->remcount, r2 = buf->p32[0],
                                       @ r3 = buf->p32[1]
    ldmia   r0, { r4-r12, r14 }        @ r4 = gain, r5-r7 = coeffs,
                                       @ r8-r11 = history, r12 = index,
                                       @ r14 = index_max
    add     r0, r0, #0x28              @ r0 = state->delay
    stmfd   sp!, { r0-r1, r14 }        @ stack state->delay, count, index_max

   /* Register usage in loop:
     * r0 = acc low/count, r1 = acc high, r2 = buf->p32[0],
     * r3 = buf->p32[1], r4 = direct gain, r5-r7 = b0, b1, a1 (filter coefs),
     * r8 = dr[n-1], r9 = y_r[n-1], r10 = dl[n-1], r11 = y_l[n-1],
     * r12 = index, r14 = scratch/index_max
     */
.cfloop:
    smull   r0, r1, r6, r8             @ acc = b1*dr[n - 1]
    ldr     r8, [r12, #4]              @ r8 = dr[n]
    smlal   r0, r1, r7, r9             @ acc += a1*y_r[n - 1]
    smlal   r0, r1, r5, r8             @ acc += b0*dr[n]
    ldr     r14, [r2]                  @ load left input: x_l[n]
    mov     r9, r1, asl #1             @ fix format for filter history
    smlal   r0, r1, r4, r14            @ acc += gain*x_l[n]
    mov     r1, r1, asl #1             @ fix format
    str     r1, [r2], #4               @ save result
    smull   r0, r1, r6, r10            @ acc = b1*dl[n - 1]
    ldr     r10, [r12]                 @ r10 = dl[n]
    smlal   r0, r1, r7, r11            @ acc += a1*y_l[n - 1]
    smlal   r0, r1, r5, r10            @ acc += b0*dl[n]
    str     r14, [r12], #4             @ save left input to delay line
    ldr     r14, [r3]                  @ load right input: x_r[n]
    mov     r11, r1, asl #1            @ fix format for filter history
    smlal   r0, r1, r4, r14            @ acc += gain*x_r[n]
    str     r14, [r12], #4             @ save right input to delay line
    ldmib   sp, { r0, r14 }            @ fetch count and delay end
    mov     r1, r1, asl #1             @ fix format
    str     r1, [r3], #4               @ save result

    cmp     r12, r14                   @ need to wrap to start of delay?
    ldrhs   r12, [sp]                  @ wrap delay index

    subs    r0, r0, #1                 @ are we finished?
    strgt   r0, [sp, #4]               @ save count to stack
    bgt     .cfloop

    @ save data back to struct
    ldr     r0, [sp]                   @ fetch state->delay
    sub     r0, r0, #0x18              @ save filter history and delay index
    stmia   r0, { r8-r12 }             @
    add     sp, sp, #12                @ remove temp variables from stack
    ldmpc   regs=r4-r11
    .size   crossfeed_process, .-crossfeed_process

/****************************************************************************
 * void crossfeed_meier_process(struct dsp_proc_entry *this,
 *                              struct dsp_buffer **buf_p)
 */
    .section .text
    .global crossfeed_meier_process
crossfeed_meier_process:
    @ input: r0 = this, r1 = buf_p
    ldr     r1, [r1]                   @ r1 = buf = *buf_p;
    ldr     r0, [r0]                   @ r0 = this->data = &crossfeed_state
    stmfd   sp!, { r4-r10, lr }        @ stack non-volatile context
    ldmia   r1, { r1-r3 }              @ r1 = buf->remcout, r2=p32[0], r3=p32[1]
    ldmib   r0, { r4-r8 }              @ r4 = vcl, r5 = vcr, r6 = vdiff
                                       @ r7 = coef1, r8 = coef2
.cfm_loop:
    ldr     r12, [r2]                  @ r12 = lout
    ldr     r14, [r3]                  @ r14 = rout
    smull   r9, r10, r8, r6            @ r9, r10  = common = coef2*vdiff
    add     r12, r12, r4               @ lout += vcl
    add     r14, r14, r5               @ rout += vcr
    sub     r6, r12, r14               @ r6 = vdiff = lout - rout
    str     r12, [r2], #4              @ store left channel
    str     r14, [r3], #4              @ store right channel
    rsbs    r12, r9, #0                @ r12 = -common (lo)
    rsc     r14, r10, #0               @ r14 = -common (hi)
    smlal   r9, r10, r7, r4            @ r9, r10  = res1 = coef1*vcl + common
    smlal   r12, r14, r7, r5           @ r12, r14 = res2 = coef1*vcr - common
    subs    r1, r1, #1                 @ count--
    mov     r9, r9, lsr #31            @ r9 = convert res1 to s0.31
    orr     r9, r9, r10, asl #1        @ .
    mov     r12, r12, lsr #31          @ r12 = convert res2 to s0.31
    orr     r12, r12, r14, asl #1      @ .
    sub     r4, r4, r9                 @ r4 = vcl -= res1
    sub     r5, r5, r12                @ r5 = vcr -= res2
    bgt     .cfm_loop                  @ more samples?

    stmib   r0, { r4-r6 }              @ save vcl, vcr, vdiff
    ldmpc   regs=r4-r10                @ restore non-volatile context, return
    .size   crossfeed_meier_process, .-crossfeed_meier_process

/****************************************************************************
 * int resample_hermite(struct resample_data *data, struct dsp_buffer *src,
 *                      struct dsp_buffer *dst)
 */
    .section    .text, "ax",%progbits
    .global     resample_hermite
resample_hermite:
    @input: r0 = data, r1 = src, r2 = dst
    stmfd   sp!, { r0-r2, r4-r11, lr }  @ stack parms, modified regs
    ldr     r9, [r1]                    @ r9 = srcrem = src->remcount
    ldrb    r10, [r1, #17]              @ r10 = ch = num_channels
    ldr     r14, [r0]                   @ r14 = data->delta, r0 = data

    cmp     r9, #0x8000                 @ srcrem = MIN(srcrem, 0x8000)
    movgt   r9, #0x8000                 @

    @ Channels are processed high to low while history is saved low to high
    @ It's really noone's business how we do this
    add     r12, r0, #8                 @ r12 = h = data->history

.hrs_channel_loop:
    stmfd   sp!, { r10, r12 }           @ push ch, h
    ldr     r5, [r0, #4]                @ r5 = data->phase
    ldr     r6, [r1, r10, lsl #2]       @ r6 = src->p32[ch]
    ldr     r7, [r2, r10, lsl #2]       @ r7 = dst->p32[ch]
    ldr     r8, [r2, #12]               @ r8 = dstrem = dst->bufcount

    mov     r0, r5, lsr #16             @ r0 = pos = phase >> 16
    cmp     r0, r9                      @ r0 = pos = MIN(pos, srcrem)
    movgt   r0, r9                      @

    add     r6, r6, r0, lsl #2          @ r6 = &s[pos]

    cmp     r0, #3                      @ pos >= 3? history not needed
    ldmgedb r6, { r1-r3 }               @ x3..x1 = s[pos-3]..s[pos-1]
    bge     .hrs_loadhist_done          @
    add     r10, r0, r0, lsl #1         @ branch pc + pos*12
    add     pc, pc, r10, lsl #2         @
    nop                                 @

    ldmia   r12, { r1-r3 }              @ x3..x1 = h[0]..h[2]
    b       .hrs_loadhist_done          @
    nop                                 @

    ldmib   r12, { r1-r2 }              @ x3..x2 = h[1]..h[2]
    ldr     r3, [r6, #-4]               @ x1 = s[0]
    b       .hrs_loadhist_done          @

    ldr     r1, [r12, #8]               @ x3 = h[2]
    ldmdb   r6, { r2-r3 }               @ x2..x1 = s[0]..s[1]
.hrs_loadhist_done:

    cmp      r0, r9                     @ pos past end?
    bge     .hrs_channel_done

    cmp     r14, #0x10000               @ delta >= 1.0?
    bhs     .hrs_dsstart                @ yes? is downsampling

    /** Upsampling **/
    str     r9, [sp, #-4]!              @ push srcrem
    mov     r5, r5, lsl #16             @ r5 = phase << 16
    sub     r0, r9, r0                  @ r0 = dte = srcrem - pos
    mov     r14, r14, lsl #16           @ r14 = delta << 16

    @ Register usage in loop:
    @ r0 = dte
    @ r1 = x3, r2 = x2, r3 = x1, r4 = x0
    @ r5 = phase << 16/frac, r6 = &s[pos], r7 = d, r8 = dstrem
    @ r9 = scratch/acclo, r10 = scratch/acchi
    @ r11 = c2, r12 = c3, c1 calculated in frac loop
    @ r14 = delta << 16
    @
    @ Try to avoid overflow as much as possible and at the same time preserve
    @ accuracy. Same formulas apply to downsampling but registers and
    @ instruction order differ due to specific constraints.
    @ c1 = -0.5*x3 + 0.5*x1
    @    = 0.5*(x1 - x3)            <--
    @
    @ v = x1 - x2, -v = x2 - x1
    @ c2 = x3 - 2.5*x2 + 2*x1 - 0.5*x0
    @    = x3 + 2*(x1 - x2) - 0.5*(x0 + x2)
    @    = x3 + 2*v - 0.5*(x0 + x2) <--
    @
    @ c3 = -0.5*x3 + 1.5*x2 - 1.5*x1 + 0.5*x0
    @    = 0.5*(x0 - x3 + (x2 - x1)) + (x2 - x1)
    @    = 0.5*(x0 - x3 - v) - v    <--
.hrs_usloop_carry:
    ldr     r4, [r6], #4                @ x0 = s[pos]
    sub     r9, r3, r2                  @ r9 = v, r11 = c2, r12 = c3
    add     r11, r1, r9, asl #1         @
    add     r10, r4, r2                 @
    sub     r12, r4, r1                 @
    sub     r12, r12, r9                @
    sub     r11, r11, r10, asr #1       @
    rsb     r12, r9, r12, asr #1        @
.hrs_usloop_frac:
    mov     r5, r5, lsr #16             @ r5 = phase -> frac
    smull   r9, r10, r12, r5            @ acc = frac * c3 + c2
    add     r9, r11, r9, lsr #16        @
    add     r9, r9, r10, asl #16        @
    smull   r9, r10, r5, r9             @ acc = frac * acc + c1
    mov     r9, r9, lsr #16             @
    orr     r9, r9, r10, asl #16        @
    sub     r10, r3, r1                 @
    add     r9, r9, r10, asr #1         @
    smull   r9, r10, r5, r9             @ acc = frac * acc + x2
    subs    r8, r8, #1                  @ destination full?
    add     r9, r2, r9, lsr #16         @
    add     r9, r9, r10, asl #16        @
    str     r9, [r7], #4                @ *d++ = acc
    bls     .hrs_usfull                 @ yes? channel is done
    adds    r5, r14, r5, lsl #16        @ frac += delta
    bcc     .hrs_usloop_frac            @ if carry is set, pos is incremented

    subs    r0, r0, #1                  @ if dte > 0, do another sample
    mov     r1, r2                      @ x3 = x2
    mov     r2, r3                      @ x2 = x1
    mov     r3, r4                      @ x1 = x0
    bgt     .hrs_usloop_carry
    b       .hrs_usdone

.hrs_usfull:
    adds    r5, r14, r5, lsl #16        @ do missed phase increment
    bcc     .hrs_usdone                 @
    sub     r0, r0, #1                  @ do missed dte decrement
    mov     r1, r2                      @ do missed history update
    mov     r2, r3                      @
    mov     r3, r4                      @

.hrs_usdone:
    ldr     r9, [sp], #4                @ r9 = pop srcrem
    mov     r14, r14, lsr #16           @ restore delta for next round
    sub     r0, r9, r0                  @ r0 = pos = srcrem - dte
    orr     r5, r5, r0                  @ reconstruct swapped phase
    mov     r5, r5, ror #16             @ swap pos and frac for phase
    b       .hrs_channel_done

    /** Downsampling **/
    @ Register usage in loop:
    @ r0 = pos/frac
    @ r1 = x3, r2 = x2, r3 = x1, r4 = x0
    @ r5 = phase, r6 = &s[pos], r7 = d, r8 = dstrem
    @ r9 = srcrem, r10 = scratch/acclo
    @ r11 = c2/scratch, r12 = c3/acchi
    @ r14 = delta
.hrs_dsloop_4:
    ldmdb   r6, { r1-r3 }               @ x3..x0 = s[pos-3]..s[pos-1]
    b       .hrs_dsloop
.hrs_dsloop_3:
    ldmdb   r6, { r2-r3 }               @ x2..x0 = s[pos-2]..s[pos-1]
    mov     r1, r4                      @ x3 = x0
    b       .hrs_dsloop
.hrs_dsloop_2:
    mov     r1, r3                      @ x3 = x1
    ldr     r3, [r6, #-4]               @ x1 = s[pos-1]
    mov     r2, r4                      @ x2 = x0
    b       .hrs_dsloop
.hrs_dsloop_1:                          @ expected loop destination
    mov     r1, r2                      @ x3 = x2
    mov     r2, r3                      @ x2 = x1
    mov     r3, r4                      @ x1 = x0
.hrs_dsloop:
    subs    r8, r8, #1                  @ destination full?
    cmpgt   r9, r0                      @ ... || pos >= srcrem?
    ble     .hrs_channel_done
.hrs_dsstart:
    ldr     r4, [r6]                    @ x0 = s[pos]
    sub     r10, r3, r2                 @ r10 = v, r11 = c2, r12 = c3
    add     r11, r4, r2                 @
    bic     r0, r5, r0, lsl #16         @ r0 = frac = phase & 0xffff
    sub     r11, r1, r11, asr #1        @
    add     r11, r11, r10, asl #1       @
    sub     r12, r4, r1                 @
    sub     r12, r12, r10               @
    rsb     r12, r10, r12, asr #1       @
    smull   r10, r12, r0, r12           @ acc = frac * c3 + c2
    add     r10, r11, r10, lsr #16      @
    add     r10, r10, r12, asl #16      @
    sub     r11, r3, r1                 @
    smull   r10, r12, r0, r10           @ acc = frac * acc + c1
    mov     r11, r11, asr #1            @
    add     r10, r11, r10, lsr #16      @
    add     r10, r10, r12, asl #16      @
    smull   r10, r12, r0, r10           @ acc = frac * acc + x2
    mov     r11, r5, lsr #16            @ r11 = last_pos
    add     r5, r5, r14                 @ phase += delta
    mov     r0, r5, lsr #16             @ r0 = pos = phase >> 16
    add     r10, r2, r10, lsr #16       @
    add     r10, r10, r12, asl #16      @
    str     r10, [r7], #4               @ *d++ = acc

    cmp     r0, r9                      @ r0 = pos = MIN(pos, srcrem)
    movgt   r0, r9                      @
    sub     r11, r0, r11                @ shift = pos - last_pos
    cmp     r11, #4                     @
    add     r6, r6, r11, lsl #2         @ r6 += shift * 4
    bge     .hrs_dsloop_4               @
    ldr     pc, [pc, r11, lsl #2]       @ branch to corresponding loop address
    .word   0, 0
    .word   .hrs_dsloop_1
    .word   .hrs_dsloop_2
    .word   .hrs_dsloop_3

.hrs_channel_done:
    ldmfd   sp!, { r10, r12 }           @ recover ch, h
    subs    r10, r10, #1                @ --ch
    stmia   r12!, { r1-r3 }             @ h[0..2] = x3..x1
    ldmgtia sp, { r0-r2 }               @ load data, src, dst
    bgt     .hrs_channel_loop

    ldmfd   sp!, { r1-r3 }              @ pop data, src, dst
    sub     r5, r5, r0, lsl #16         @ r5 = phase - (pos << 16)
    ldr     r2, [r3, #12]               @ r2 = dst->bufcount
    str     r5, [r1, #4]                @ data->phase = r5
    sub     r2, r2, r8                  @ r2 = dst->bufcount - dstrem
    str     r2, [r3]                    @ dst->remcount = r2
    ldmpc   regs=r4-r11                 @ ... and we're out
    .size   resample_hermite, .-resample_hermite

/****************************************************************************
 *  void pga_process(struct dsp_proc_entry *this, struct dsp_buffer **buf_p)
 */
    .section    .text, "ax",%progbits
    .global pga_process
    .type   pga_process, %function
pga_process:
    @ input: r0 = this, r1 = buf_p
    ldr     r0, [r0]                @ r0 = data = this->data (&pga_data)
    ldr     r1, [r1]                @ r1 = buf = *buf_p;
    stmfd   sp!, { r4-r8, lr }

    ldr     r4, [r0]                @ r4 = data->gain
    ldr     r0, [r1], #4            @ r0 = buf->remcount, r1 = buf->p32
    ldrb    r3, [r1, #13]           @ r3 = buf->format.num_channels

.pga_channelloop:
    ldr     r2, [r1], #4            @ r2 = buf->p32[ch] and inc index of p32
    subs    r12, r0, #1             @ r12 = count - 1
    beq     .pga_singlesample       @ Zero? Only one sample!

.pga_loop:
    ldmia   r2, { r5, r6 }          @ load r5, r6 from r2 (*p32[ch])
    smull   r7, r8, r5, r4          @ r7 = FRACMUL_SHL(r5, r4, 8)
    smull   r14, r5, r6, r4         @ r14 = FRACMUL_SHL(r6, r4, 8)
    subs    r12, r12, #2
    mov     r7, r7, lsr #23
    mov     r14, r14, lsr #23
    orr     r7, r7, r8, asl #9
    orr     r14, r14, r5, asl #9
    stmia   r2!, { r7, r14 }        @ save r7, r14 to *p32[ch] and increment
    bgt     .pga_loop               @ end of pga loop

    blt     .pga_evencount          @ < 0? even count

.pga_singlesample:
    ldr     r5, [r2]                @ handle odd sample
    smull   r7, r8, r5, r4          @ r7 = FRACMUL_SHL(r5, r4, 8)
    mov     r7, r7, lsr #23
    orr     r7, r7, r8, asl #9
    str     r7, [r2]

.pga_evencount:
    subs    r3, r3, #1
    bgt     .pga_channelloop        @ end of channel loop

    ldmpc   regs=r4-r8
    .size   pga_process, .-pga_process

/****************************************************************************
 * void filter_process(struct dsp_filter *f, int32_t *buf[], int count,
 *                     unsigned int channels)
 *
 * define HIGH_PRECISION as '1' to make filtering calculate lower bits after
 * shifting. without this, "shift" - 1 of the lower bits will be lost here.
 */
#define HIGH_PRECISION 0

#if CONFIG_CPU == PP5002
    .section    .icode,"ax",%progbits
#else
    .section    .text, "ax",%progbits
#endif
    .global filter_process
filter_process:
    @input: r0 = f, r1 = buf, r2 = count, r3 = channels
    stmfd   sp!, { r4-r11, lr }     @ save all clobbered regs
    ldmia   r0!, { r4-r8 }          @ load coefs, r0 = f->history
    sub     r3, r3, #1              @ r3 = ch = channels - 1
    stmfd   sp!, { r0-r3 }          @ save adjusted params
    ldrb    r14, [r0, #32]          @ r14 = shift

    @ Channels are processed high to low while history is saved low to high
    @ It's really noone's business how we do this
.fp_channelloop:
    ldmia   r0, { r9-r12 }          @ load history, r0 = history[channels-ch-1]
    ldr     r3, [r1, r3, lsl #2]    @ r3 = buf[ch]

    @ r9-r12 = history, r4-r8 = coefs, r0..r1 = accumulator,
    @ r2 = number of samples, r3 = buf[ch], r14 = shift amount
.fp_loop:
    @ Direct form 1 filtering code.
    @ y[n] = b0*x[i] + b1*x[i - 1] + b2*x[i - 2] + a1*y[i - 1] + a2*y[i - 2],
    @ where y[] is output and x[] is input. This is performed out of order to
    @ reuse registers, we're pretty short on regs.
    smull   r0, r1, r5, r9          @ acc = b1*x[i - 1]
    smlal   r0, r1, r6, r10         @ acc += b2*x[i - 2]
    mov     r10, r9                 @ fix input history
    ldr     r9, [r3]                @ load input and fix history
    smlal   r0, r1, r7, r11         @ acc += a1*y[i - 1]
    smlal   r0, r1, r8, r12         @ acc += a2*y[i - 2]
    smlal   r0, r1, r4, r9          @ acc += b0*x[i] /* avoid stall on arm9 */
    mov     r12, r11                @ fix output history
    mov     r11, r1, asl r14        @ get upper part of result and shift left
#if HIGH_PRECISION
    rsb     r1, r14, #32            @ get shift amount for lower part
    orr     r11, r11, r0, lsr r1    @ then mix in correctly shifted lower part
#endif
    str     r11, [r3], #4           @ save result
    subs    r2, r2, #1              @ are we done with this channel?
    bgt     .fp_loop                @

    ldr     r3, [sp, #12]           @ r3 = ch
    ldr     r0, [sp]                @ r0 = history[channels-ch-1]
    subs    r3, r3, #1              @ all channels processed?
    stmia   r0!, { r9-r12 }         @ save back history, history++
    ldmhsib sp, { r1-r2 }           @ r1 = buf, r2 = count
    strhs   r3, [sp, #12]           @ store ch
    strhs   r0, [sp]                @ store history[channels-ch-1]
    bhs     .fp_channelloop

    add     sp, sp, #16             @ compensate for temp storage
    ldmpc   regs=r4-r11
    .size   filter_process, .-filter_process

#if ARM_ARCH < 6
/****************************************************************************
 *  void sample_output_mono(struct sample_io_data *this,
 *                          struct dsp_buffer *src,
 *                          struct dsp_buffer *dst)
 */
    .section    .icode,"ax",%progbits
    .global sample_output_mono
    .type   sample_output_mono, %function
sample_output_mono:
    @ input: r0 = this, r1 = src, r2 = dst
    stmfd   sp!, { r4-r6, lr }

    ldr     r0, [r0]                   @ r0 = this->outcount
    ldr     r3, [r2, #4]               @ r2 = dst->p16out
    ldr     r2, [r1, #4]               @ r1 = src->p32[0]
    ldrb    r1, [r1, #19]              @ r2 = src->format.output_scale

    mov     r4, #1
    mov     r4, r4, lsl r1             @ r4 = 1 << (scale-1)
    mov     r4, r4, lsr #1
    mvn     r14, #0x8000               @ r14 = 0xffff7fff, needed for
                                       @ clipping and masking
    subs    r0, r0, #1                 @
    beq     .som_singlesample          @ Zero? Only one sample!

.somloop:
    ldmia   r2!, { r5, r6 }
    add     r5, r5, r4                 @ r6 = (r6 + 1<<(scale-1)) >> scale
    mov     r5, r5, asr r1
    mov     r12, r5, asr #15
    teq     r12, r12, asr #31
    eorne   r5, r14, r5, asr #31       @ Clip (-32768...+32767)
    add     r6, r6, r4
    mov     r6, r6, asr r1             @ r7 = (r7 + 1<<(scale-1)) >> scale
    mov     r12, r6, asr #15
    teq     r12, r12, asr #31
    eorne   r6, r14, r6, asr #31       @ Clip (-32768...+32767)

    and     r5, r5, r14, lsr #16
    and     r6, r6, r14, lsr #16
    orr     r5, r5, r5, lsl #16        @ pack first 2 halfwords into 1 word
    orr     r6, r6, r6, lsl #16        @ pack last 2 halfwords into 1 word
    stmia   r3!, { r5, r6 }

    subs    r0, r0, #2
    bgt     .somloop

    ldmpc   cond=lt, regs=r4-r6        @ even 'count'? return

.som_singlesample:
    ldr     r5, [r2]                   @ do odd sample
    add     r5, r5, r4
    mov     r5, r5, asr r1
    mov     r12, r5, asr #15
    teq     r12, r12, asr #31
    eorne   r5, r14, r5, asr #31

    and     r5, r5, r14, lsr #16       @ pack 2 halfwords into 1 word
    orr     r5, r5, r5, lsl #16
    str     r5, [r3]

    ldmpc   regs=r4-r6
    .size   sample_output_mono, .-sample_output_mono

/****************************************************************************
 *  void sample_output_stereo(struct sample_io_data *this,
 *                          struct dsp_buffer *src,
 *                          struct dsp_buffer *dst)
 */
    .section    .icode,"ax",%progbits
    .global sample_output_stereo
    .type   sample_output_stereo, %function
sample_output_stereo:
    @ input: r0 = this, r1 = src, r2 = dst
    stmfd   sp!, { r4-r9, lr }

    ldr     r0, [r0]                   @ r0 = this->outcount
    ldr     r3, [r2, #4]               @ r3 = dsp->p16out
    ldmib   r1, { r2, r5 }             @ r2 = src->p32[0], r5 = src->p32[1]
    ldrb    r1, [r1, #19]              @ r1 = src->format.output_scale

    mov     r4, #1
    mov     r4, r4, lsl r1             @ r4 = 1 << (scale-1)
    mov     r4, r4, lsr #1             @

    mvn     r14, #0x8000               @ r14 = 0xffff7fff, needed for
                                       @ clipping and masking
    subs    r0, r0, #1                 @
    beq     .sos_singlesample          @ Zero? Only one sample!

.sosloop:
    ldmia   r2!, { r6, r7 }            @ 2 left
    ldmia   r5!, { r8, r9 }            @ 2 right

    add     r6, r6, r4                 @ r6 = (r6 + 1<<(scale-1)) >> scale
    mov     r6, r6, asr r1
    mov     r12, r6, asr #15
    teq     r12, r12, asr #31
    eorne   r6, r14, r6, asr #31       @ Clip (-32768...+32767)
    add     r7, r7, r4
    mov     r7, r7, asr r1             @ r7 = (r7 + 1<<(scale-1)) >> scale
    mov     r12, r7, asr #15
    teq     r12, r12, asr #31
    eorne   r7, r14, r7, asr #31       @ Clip (-32768...+32767)

    add     r8, r8, r4                 @ r8 = (r8 + 1<<(scale-1)) >> scale
    mov     r8, r8, asr r1
    mov     r12, r8, asr #15
    teq     r12, r12, asr #31
    eorne   r8, r14, r8, asr #31       @ Clip (-32768...+32767)
    add     r9, r9, r4                 @ r9 = (r9 + 1<<(scale-1)) >> scale
    mov     r9, r9, asr r1
    mov     r12, r9, asr #15
    teq     r12, r12, asr #31
    eorne   r9, r14, r9, asr #31       @ Clip (-32768...+32767)

    and     r6, r6, r14, lsr #16       @ pack first 2 halfwords into 1 word
    orr     r8, r6, r8, asl #16
    and     r7, r7, r14, lsr #16       @ pack last 2 halfwords into 1 word
    orr     r9, r7, r9, asl #16

    stmia   r3!, { r8, r9 }

    subs    r0, r0, #2
    bgt     .sosloop

    ldmpc   cond=lt, regs=r4-r9        @ even 'count'? return

.sos_singlesample:
    ldr     r6, [r2]                   @ left odd sample
    ldr     r8, [r5]                   @ right odd sample

    add     r6, r6, r4                 @ r6 = (r7 + 1<<(scale-1)) >> scale
    mov     r6, r6, asr r1
    mov     r12, r6, asr #15
    teq     r12, r12, asr #31
    eorne   r6, r14, r6, asr #31       @ Clip (-32768...+32767)
    add     r8, r8, r4                 @ r8 = (r8 + 1<<(scale-1)) >> scale
    mov     r8, r8, asr r1
    mov     r12, r8, asr #15
    teq     r12, r12, asr #31
    eorne   r8, r14, r8, asr #31       @ Clip (-32768...+32767)

    and     r6, r6, r14, lsr #16       @ pack 2 halfwords into 1 word
    orr     r8, r6, r8, asl #16

    str     r8, [r3]

    ldmpc   regs=r4-r9
    .size   sample_output_stereo, .-sample_output_stereo
#endif /* ARM_ARCH < 6 */
