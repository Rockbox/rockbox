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
 #include "config.h"

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
    ldr     r12, [r1]                  @ r12 = buf->remcount
    ldr     r14, [r0]                  @ r14 = this->data = &crossfeed_state
    ldmib   r1, { r2-r3 }              @ r2 = buf->p32[0], r3 = buf->p32[1]
    ldmia   r14!, { r4-r11 }           @ load direct gain and filter data
    add     r0, r14, #13*2*4           @ calculate end of delay
    stmfd   sp!, { r0, r12 }           @ stack end of delay adr, count and state
    ldr     r0, [r0]                   @ fetch current delay line address

    /* Register usage in loop:
     * r0 = &delay[index][0], r1 = accumulator high, r2 = buf->p32[0],
     * r3 = buf->p32[1], r4 = direct gain, r5-r7 = b0, b1, a1 (filter coefs),
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
    ldmia   sp, { r12, r14 }           @ fetch delay line end addr and count from stack
    str     r1, [r3], #4               @ save result

    cmp     r0, r12                    @ need to wrap to start of delay?
    subhs   r0, r12, #13*2*4           @ wrap back delay line ptr to start

    subs    r14, r14, #1               @ are we finished?
    strgt   r14, [sp, #4]              @ nope, save count back to stack
    bgt     .cfloop

    @ save data back to struct
    str     r0, [r12]                  @ save delay line index
    sub     r12, r12, #13*2*4 + 4*4    @ r12 = data->history
    stmia   r12, { r8-r11 }            @ save filter history
    add     sp, sp, #8                 @ remove temp variables from stack
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
    add     r0, r0, #16                @ r0 = &state->vcl
    ldmia   r0, { r4-r8 }              @ r4 = vcl, r5 = vcr, r6 = vdiff
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

    stmia   r0, { r4-r6 }              @ save vcl, vcr, vdiff
    ldmpc   regs=r4-r10                @ restore non-volatile context, return
    .size   crossfeed_meier_process, .-crossfeed_meier_process


/****************************************************************************
 * int lin_resample_resample(struct resample_data *data,
 *                           struct dsp_buffer *src,
 *                           struct dsp_buffer *dst)
 */
    .section    .text, "ax",%progbits
    .global     lin_resample_resample
lin_resample_resample:
    @input: r0 = data, r1 = src, r2 = dst
    stmfd   sp!, { r4-r11, lr }     @ stack modified regs
    ldr     r4, [r0]                @ r4 = data->delta
    add     r10, r0, #4             @ r10 = &data->phase
    ldrb    r3, [r1, #17]           @ r3 = num_channels,
    stmfd   sp!, { r1, r10 }        @ stack src, &data->phase
.lrs_channel_loop:
    ldr     r5, [r10]               @ r5 = data->phase
    ldr     r6, [r1]                @ r6 = srcrem = src->remcount
    ldr     r7, [r1, r3, lsl #2]    @ r7 = src->p32[ch]
    ldr     r8, [r2, r3, lsl #2]    @ r8 = dst->p32[ch]
    ldr     r9, [r2, #12]           @ r9 = dstrem = dst->bufcount

    cmp     r6, #0x8000             @ srcrem = MIN(srcrem, 0x8000)
    movgt   r6, #0x8000             @
    mov     r0, r5, lsr #16         @ pos = MIN(pos, srcrem)
    cmp     r0, r6                  @
    movgt   r0, r6                  @ r0 = pos = phase >> 16
    cmp     r0, #0                  @
    ldrle   r11, [r10, r3, lsl #2]  @ pos <= 0? r11 = last = last_sample[ch]
    addgt   r12, r7, r0, lsl #2     @ pos > 0? r1 = last = s[pos - 1]
    ldrgt   r11, [r12, #-4]         @
    cmp     r0, r6                  @
    bge     .lrs_channel_done       @ pos >= count? channel complete

    cmp     r4, #0x10000            @ delta >= 1.0?
    ldrhs   r12, [r7, r0, lsl #2]   @ yes? r12 = s[pos]
    bhs     .lrs_dsstart            @ yes? is downsampling

    /** Upsampling **/
    mov     r5, r5, lsl #16         @ Move phase into high halfword
    add     r7, r7, r0, lsl #2      @ r7 = &s[pos]
    sub     r0, r6, r0              @ r0 = dte = srcrem - pos
.lrs_usloop_1:
    ldr     r12, [r7], #4           @ r12 = s[pos]
    sub     r14, r12, r11           @ r14 = diff = s[pos] - s[pos - 1]
.lrs_usloop_0:
    mov     r1, r5, lsr #16         @ r1 = frac = phase >> 16
    @ keep frac in Rs to take advantage of multiplier early termination
    smull   r1, r10, r14, r1        @ r1, r10 = diff * frac (lo, hi)
    add     r1, r11, r1, lsr #16    @ r1 = out = last + frac*diff
    add     r1, r1, r10, lsl #16    @
    str     r1, [r8], #4            @ *d++ = out
    subs    r9, r9, #1              @ destination full?
    bls     .lrs_usfull             @ yes? channel is done
    adds    r5, r5, r4, lsl #16     @ phase += delta << 16
    bcc     .lrs_usloop_0           @ if carry is set, pos is incremented
    subs    r0, r0, #1              @ if srcrem > 0, do another sample
    mov     r11, r12                @ r11 = last = s[pos-1] (pos changed)
    bgt     .lrs_usloop_1
    b       .lrs_usdone

.lrs_usfull:
    adds    r5, r5, r4, lsl #16     @ do missed phase increment
    subcs   r0, r0, #1              @ do missed srcrem decrement
    movcs   r11, r12                @ r11 = s[pos-1] (pos changed)

.lrs_usdone:
    sub     r0, r6, r0              @ r0 = pos = srcrem - dte
    orr     r5, r5, r0              @ reconstruct swapped phase
    mov     r5, r5, ror #16         @ swap pos and frac for phase
    b       .lrs_channel_done       @

    /** Downsampling **/
.lrs_dsloop:
    add     r10, r7, r0, lsl #2     @ r10 = &s[pos]
    ldmda   r10, { r11, r12 }       @ r11 = last, r12 = s[pos]
.lrs_dsstart:
    sub     r14, r12, r11           @ r14 = diff = s[pos] - s[pos - 1]
    @ keep frac in Rs to take advantage of multiplier early termination
    bic     r1, r5, r0, lsl #16     @ frac = phase & 0xffff
    smull   r1, r10, r14, r1        @ r1, r10 = diff * frac (lo, hi)
    add     r5, r5, r4              @ phase += delta
    subs    r9, r9, #1              @ destination full? ...
    mov     r0, r5, lsr #16         @ pos = phase >> 16
    add     r1, r11, r1, lsr #16    @ r1 = out = last + frac*diff
    add     r1, r1, r10, lsl #16    @
    str     r1, [r8], #4            @ *d++ = out
    cmpgt   r6, r0                  @ ... || pos >= srcrem? ...
    bgt     .lrs_dsloop             @ ... no, do more samples

    cmp     r0, r6                  @ pos = MIN(pos, srcrem)
    movgt   r0, r6                  @
    sub     r1, r0, #1              @ pos must always be > 0 since step >= 1.0
    ldr     r11, [r7, r1, lsl #2]   @ r11 = s[pos - 1]

.lrs_channel_done:
    ldmia   sp, { r1, r10 }         @ recover src, &data->phase
    str     r11, [r10, r3, lsl #2]  @ last_sample[ch] = last
    subs    r3, r3, #1              @
    bgt     .lrs_channel_loop       @

    ldr     r6, [r2, #12]           @ r6 = dst->bufcount
    sub     r5, r5, r0, lsl #16     @ r5 = phase - (pos << 16)
    str     r5, [r10]               @ data->phase = r5
    sub     r6, r6, r9              @ r6 = dst->bufcount - dstrem = dstcount
    str     r6, [r2]                @ dst->remcount = dstcount
    add     sp, sp, #8              @ adjust stack for temp variables
    ldmpc   regs=r4-r11             @ ... and we're out
    .size   lin_resample_resample, .-lin_resample_resample

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
