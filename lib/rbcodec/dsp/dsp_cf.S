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
 * Copyright (C) 2007, 2012 Michael Sevakis
 * Copyright (C) 2010 Bertrik Sikken
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
 * void pga_process(struct dsp_proc_entry *this, struct dsp_buffer **buf_p)
 */
    .section    .text
    .align      2
    .global     pga_process
pga_process:
    | input: 4(sp) = this, 8(sp) = buf_p
    movem.l     4(%sp), %a0-%a1         | %a0 = this, %a1 = buf_p
    move.l      (%a0), %a0              | %a0 = this->data = &pga_data
    move.l      (%a0), %a0              | %a0 = data->gain
    move.l      (%a1), %a1              | %a1 = buf = *buf_p
    lea.l       -20(%sp), %sp           | save registers
    movem.l     %d2-%d4/%a2-%a3, (%sp)  |
    clr.l       %d1                     | %d1 = buf->format.num_channels
    move.b      17(%a1), %d1            |
10: | channel loop                      |
    move.l      (%a1), %d0              | %d0 = buf->remcount
    move.l      (%a1, %d1.l*4), %a2     | %a2 = s = buf->p32[ch-1]
    move.l      %a2, %a3                | %a3 = d = s
    move.l      (%a2)+, %d2             | %d2 = *s++,
    mac.l       %a0, %d2, (%a2)+, %d2, %acc0 | %acc0 = S(n)*gain, load S(n+1)
    subq.l      #1, %d0                 | --count > 0 ? : effectively n++
    ble.b       30f | loop done         | no? finish up
20: | loop                              |
    move.l      %accext01, %d4          | fetch S(n-1)[7:0]
    movclr.l    %acc0, %d3              | fetch S(n-1)[40:8] in %d5[31:0]
    asl.l       #8, %d3                 | *s++ = (S(n-1)[40:8] << 8) | S(n-1)[7:0]
    mac.l       %a0, %d2, (%a2)+, %d2, %acc0 | %acc0 = S(n)*gain, load S(n+1)
    move.b      %d4, %d3                |
    move.l      %d3, (%a3)+             |
    subq.l      #1, %d0                 | --count > 0 ? : effectively n++
    bgt.b       20b | loop              | yes? do more samples
30: | loop done                         |
    move.l      %accext01, %d4          | fetch S(n-1)[7:0]
    movclr.l    %acc0, %d3              | fetch S(n-1)[40:8] in %d5[31:0]
    asl.l       #8, %d3                 | *s = (S(n-1)[40:8] << 8) | S(n-1)[7:0]
    move.b      %d4, %d3                |
    move.l      %d3, (%a3)              |
    subq.l      #1, %d1                 | next channel
    bgt.b       10b | channel loop      |
    movem.l     (%sp), %d2-%d4/%a2-%a3  | restore registers
    lea.l       20(%sp), %sp            | cleanup stack
    rts                                 |
    .size       pga_process, .-pga_process

/****************************************************************************
 * void crossfeed_process(struct dsp_proc_entry *this,
 *                        struct dsp_buffer **buf_p)
 */
    .section    .text
    .align      2
    .global     crossfeed_process
crossfeed_process:
    | input: 4(sp) = this, 8(sp) = buf_p
    lea.l       -44(%sp), %sp           |
    movem.l     %d2-%d7/%a2-%a6, (%sp)  | save all regs
    movem.l     48(%sp), %a1/%a4        | %a1 = this, %a4 = buf_p
    move.l      (%a4), %a4              | %a4 = buf = *buf_p
    movem.l     (%a4), %d7/%a4-%a5      | %d7 = buf->remcount, %a4 = buf->p32[0],
                                        | %a5 = buf->p32[1]
    move.l      (%a1), %a1              | %a1 = &crossfeed_state
    move.l      (%a1)+, %d6             | %d6 = direct gain
    movem.l     12(%a1), %d0-%d3        | fetch filter history samples
    lea.l       132(%a1), %a6           | %a6 = delay line wrap limit
    move.l      (%a6), %a0              | fetch delay line address
    movem.l     (%a1), %a1-%a3          | load filter coefs
    bra.b       20f | loop start        | go to loop start point
    /* Register usage in loop:
     * %a0 = delay_p, %a1..%a3 = b0, b1, a1 (filter coefs),
     * %a4 = buf[0], %a5 = buf[1],
     * %a6 = delay line pointer wrap limit,
     * %d0..%d3 = history
     * %d4..%d5 = temp.
     * %d6 = direct gain,
     * %d7 = count
     */
10: | loop                              |
    movclr.l    %acc0, %d4              | write outputs
    move.l      %d4, (%a4)+             | .
    movclr.l    %acc1, %d5              | .
    move.l      %d5, (%a5)+             | .
20: | loop start                        |
    mac.l       %a2, %d0, (%a0)+, %d0, %acc0 | %acc0  = b1*dl[n - 1], %d0 = dl[n]
    mac.l       %a1, %d0             , %acc0 | %acc0 += b0*dl[n]
    mac.l       %a3, %d1, (%a5),  %d5, %acc0 | %acc0 += a1*y_r[n - 1], load R
    mac.l       %a2, %d2, (%a0)+, %d2, %acc1 | %acc1  = b1*dr[n - 1], %d2 = dr[n]
    mac.l       %a1, %d2             , %acc1 | %acc1 += b0*dr[n]
    mac.l       %a3, %d3, (%a4),  %d4, %acc1 | %acc1 += a1*y_l[n - 1], load L
    movem.l     %d4-%d5, -8(%a0)        | save left & right inputs to delay line
    move.l      %acc0, %d3              | get filtered delayed left sample (y_l[n])
    move.l      %acc1, %d1              | get filtered delayed right sample (y_r[n])
    mac.l       %d6, %d4, %acc0         | %acc0 += gain*x_l[n]
    mac.l       %d6, %d5, %acc1         | %acc1 += gain*x_r[n]
    cmp.l       %a6, %a0                | wrap %a0 if passed end
    bhs.b       30f | wrap buffer       |
    tpf.l                               | trap the buffer wrap
30: | wrap buffer                       | ...fwd taken branches more costly
    lea.l       -104(%a6), %a0          | wrap it up
    subq.l      #1, %d7                 | --count > 0 ?
    bgt.b       10b | loop              | yes? do more
    movclr.l    %acc0, %d4              | write last outputs
    move.l      %d4, (%a4)              | .
    movclr.l    %acc1, %d5              | .
    move.l      %d5, (%a5)              | .
    movem.l     %d0-%d3, -120(%a6)      | ...history
    move.l      %a0, (%a6)              | ...delay_p
    movem.l     (%sp), %d2-%d7/%a2-%a6  | restore all regs
    lea.l       44(%sp), %sp            |
    rts                                 |
    .size       crossfeed_process,.-crossfeed_process

/****************************************************************************
 * void crossfeed_meier_process(struct dsp_proc_entry *this,
 *                              struct dsp_buffer **buf_p)
 */
    .section .text
    .global crossfeed_meier_process
crossfeed_meier_process:
    | input: 4(sp) = this, 8(sp) = buf_p
    movem.l     4(%sp), %a0-%a1         | %a0 = this, %a1 = buf_p
    lea.l       -24(%sp), %sp           | save non-volatiles
    movem.l     %d2-%d6/%a2, (%sp)      | .
    move.l      (%a0), %a0              | %a0 = &this->data = &crossfeed_state
    move.l      (%a1), %a1              | %a1 = buf = *buf_p
    movem.l     16(%a0), %d1-%d5        | %d1 = vcl, %d2 = vcr, %d3 = vdiff,
                                        | %d4 = coef1, %d5 = coef2
    movem.l     (%a1), %d0/%a1-%a2      | %d0 = count = buf->remcount
                                        | %a1 = p32[0], %a2 = p32[1]
    | Register usage in loop:
    | %d0 = count, %d1 = vcl, %d2 = vcr, %d3 = vdiff/lout,
    | %d4 = coef1, %d5 = coef2, %d6 = rout/scratch
    | %a1 = p32[0], %a2 = p32[1]
10: | loop
    mac.l       %d5, %d3, %acc0         | %acc0 = common = coef2*vdiff
    move.l      %acc0, %acc1            | copy common
    mac.l       %d4, %d1, (%a1), %d3, %acc0 | %acc0 += coef1*vcl, %d3 = lout
    msac.l      %d4, %d2, (%a2), %d6, %acc1 | %acc1 -= coef1*vcr, %d6 = rout
    add.l       %d1, %d3                | lout += vcl
    add.l       %d2, %d6                | rout += vcr
    move.l      %d3, (%a1)+             | store left channel, pos inc
    move.l      %d6, (%a2)+             | store right channel, pos inc
    sub.l       %d6, %d3                | vdiff = lout - rout
    movclr.l    %acc0, %d6              | %d4 = fetch res1 in s0.31
    sub.l       %d6, %d1                | vcl -= res1
    movclr.l    %acc1, %d6              | %d5 = fetch -res2 in s0.31
    add.l       %d6, %d2                | vcr += -res2
    subq.l      #1, %d0                 | count--
    bgt         10b | loop              | more samples?
                                        |
    movem.l     %d1-%d3, 16(%a0)        | save vcl, vcr, vdiff
    movem.l     (%sp), %d2-%d6/%a2      | restore non-volatiles
    lea.l       24(%sp), %sp            | .
    rts                                 |
    .size   crossfeed_meier_process, .-crossfeed_meier_process

/****************************************************************************
 * int lin_resample_resample(struct resample_data *data,
 *                           struct dsp_buffer *src,
 *                           struct dsp_buffer *dst)
 */
    .section    .text
    .align      2
    .global     lin_resample_resample
lin_resample_resample:
    | input: 4(sp) = data, 8(sp) = src, 12(sp) = dst
    lea.l       -44(%sp), %sp           | save non-volatiles
    movem.l     %d2-%d7/%a2-%a6, (%sp)  |
    movem.l     48(%sp), %a0-%a2        | %a0 = data
                                        | %a1 = src
                                        | %a2 = dst
    clr.l       %d1                     | %d1 = ch = src->format.num_channels
    move.b      17(%a1), %d1            |
    moveq.l     #16, %d7                | %d7 = shift
.lrs_channel_loop:                      |
    movem.l     (%a0), %d2-%d3          | %d2 = delta = data->delta,
                                        | %d3 = phase = data->phase
    move.l      (%a1), %d4              | %d4 = srcrem = src->remcount
    move.l      12(%a2), %d5            | %d5 = dstrem = dst->bufcount
    cmp.l       #0x8000, %d4            | %d4 = MIN(srcrem, 0x8000)
    ble.b       10f                     |
    move.l      #0x8000, %d4            |
10:                                     |
    move.l      (%a1, %d1.l*4), %a3     | %a3 = s = src->p32[ch]
    move.l      (%a2, %d1.l*4), %a4     | %a4 = d = dst->p32[ch]
    move.l      %d3, %d0                | %d0 = pos
    lsr.l       %d7, %d0                | ...
    beq.b       11f                     | pos == 0?
    cmp.l       %d4, %d0                | pos = MIN(pos, srcrem)
    blt.b       12f                     |
    move.l      %d4, %d0                | pos = srcrem
    move.l      -4(%a3, %d0.l*4), %d6   | %d6 = last = s[pos - 1]
    bra.w       .lrs_channel_complete   | at limit; nothing to do but next
11:                                     |
    move.l      4(%a0, %d1.l*4), %d6    | %d6 = last = last_sample[ch]
    tpf.l                               | trap next move.l (last = s[pos - 1])
12:                                     |
    move.l      -4(%a3, %d0.l*4), %d6   | %d6 = last = s[pos - 1]
    cmp.l       #0x10000, %d2           | delta >= 1.0?
    bhs.b       .lrs_downsample         | yes? downsampling
                                        |
    /** Upsampling **/                  |
    lea.l       (%a3, %d0.l*4), %a3     | %a3 = &s[pos]
    sub.l       %d4, %d0                | %d0 = pos - srcrem = -dte
    lsl.l       %d7, %d2                | move delta to bits 30..15
    lsr.l       #1, %d2                 |
    lsl.l       %d7, %d3                | move phase to bits 30..15
    lsr.l       #1, %d3                 |
    move.l      (%a3)+, %a5             | %a5 = s[pos]
    move.l      %a5, %a6                | %a6 = diff = s[pos] - last
    sub.l       %d6, %a6                |
    bra.b       22f                     |
    /* Funky loop structure is to avoid emac latency stalls */
20:                                     |
    move.l      (%a3)+, %a5             | %a5 = s[pos]
    move.l      %a5, %a6                | %a6 = diff = s[pos] - last
    sub.l       %d6, %a6                |
21:                                     |
    movclr.l    %acc0, %d7              | *d++ = %d7 = result
    move.l      %d7, (%a4)+             |
22:                                     |
    move.l      %d6, %acc0              | %acc0 = last
    mac.l       %d3, %a6, %acc0         | %acc0 += frac * diff
    subq.l      #1, %d5                 | dstrem <= 0?
    ble.b       23f                     | yes? stop
    add.l       %d2, %d3                | phase += delta
    bpl.b       21b                     | load next values?
    move.l      %a5, %d6                |
    bclr.l      #31, %d3                | clear sign bit
    addq.l      #1, %d0                 | dte > 0?
    bmi.b       20b                     | yes? continue resampling
    tpf.w                               | trap next add.l (phase += delta)
23:                                     |
    add.l       %d2, %d3                | phase += delta
    lsl.l       #1, %d3                 | frac -> phase
    bcs.b       24f                     | was sign bit set?
    tpf.l                               |
24:                                     |
    move.l      %a5, %d6                | yes? was going to move to new s[pos]
    addq.l      #1, %d0                 |
    movclr.l    %acc0, %d7              | *d = %d7 = result
    move.l      %d7, (%a4)              |
    add.l       %d4, %d0                | %d0 = -dte + srcrem = pos
    or.l        %d0, %d3                | restore phase
    swap.w      %d3                     |
    moveq.l     #16, %d7                | %d7 = shift
    bra.b       .lrs_channel_complete   |
                                        |
    /** Downsampling **/                |
.lrs_downsample:                        |
    move.l      (%a3, %d0.l*4), %a5     | %a5 = s[pos]
    bra.b       31f                     |
30:                                     |
    lea.l       -4(%a3, %d0.l*4), %a5   | %d6 = s[pos - 1], %a5 = s[pos]
    movem.l     (%a5), %d6/%a5          |
31:                                     |
    move.l      %d6, %acc0              | %acc0 = last
    sub.l       %d6, %a5                | %a5 = diff = s[pos] - s[pos - 1]
    move.l      %d3, %d0                | frac = (phase << 16) >> 1
    lsl.l       %d7, %d0                |
    lsr.l       #1, %d0                 |
    mac.l       %d0, %a5, %acc0         | %acc0 += frac * diff
    add.l       %d2, %d3                | phase += delta
    move.l      %d3, %d0                | pos = phase >> 16
    lsr.l       %d7, %d0                |
    movclr.l    %acc0, %a5              |
    move.l      %a5, (%a4)+             | *d++ = %d0
    subq.l      #1, %d5                 | dst full?
    ble.b       32f                     | yes? stop
    cmp.l       %d4, %d0                | pos < srcrem?
    blt.b       30b                     | yes? continue resampling
    tpf.l                               | trap cmp.l and ble.b
32:                                     |
    cmp.l       %d4, %d0                | pos = MIN(pos, srcrem)
    ble.b       33f                     |
    move.l      %d4, %d0                |
33:                                     |
    move.l      -4(%a3, %d0.l*4), %d6   | %d6 = s[pos - 1]
                                        |
.lrs_channel_complete:                  |
    move.l      %d6, 4(%a0, %d1.l*4)    | last_sample[ch] = last
    subq.l      #1, %d1                 | ch > 0?
    bgt.w       .lrs_channel_loop       | yes? process next channel
                                        |
    move.l      12(%a2), %d1            | %d1 = dst->bufcount
    sub.l       %d5, %d1                | written = dst->bufcount - dstrem
    move.l      %d1, (%a2)              | dst->remcount = written
    move.l      %d0, %d1                | wrap phase to position in next frame
    lsl.l       %d7, %d1                | data->phase = phase - (pos << 16)
    sub.l       %d1, %d3                | ...
    move.l      %d3, 4(%a0)             | ...
    movem.l     (%sp), %d2-%d7/%a2-%a6  | restore non-volatiles
    lea.l       44(%sp), %sp            | cleanup stack
    rts                                 | buh-bye

    .size       lin_resample_resample, .-lin_resample_resample


/****************************************************************************
 * void channel_mode_proc_mono(struct dsp_proc_entry *this,
 *                             struct dsp_buffer **buf_p)
 *
 * Mix left and right channels 50/50 into a center channel.
 */
    .section    .text
    .align      2
    .global     channel_mode_proc_mono
channel_mode_proc_mono:
    | input: 4(sp) = this, 8(sp) = buf_p
    move.l      8(%sp), %a0             | %a0 = buf_p
    move.l      (%a0), %a0              | %a0 = buf = *buf_p
    lea.l       -20(%sp), %sp           | save registers
    movem.l     %d2-%d4/%a2-%a3, (%sp)  |
    movem.l     (%a0), %d0/%a0-%a1      | %d0 = buf->remcount, %a0 = buf->p32[0],
                                        | %a1 = buf->p32[1]
    move.l      %a0, %a2                | use separate dst pointers since read
    move.l      %a1, %a3                | pointers run one ahead of write
    move.l      #0x40000000, %d3        | %d3 = 0.5
    move.l      (%a0)+, %d1             | prime the input registers
    move.l      (%a1)+, %d2             |
    mac.l       %d1, %d3, (%a0)+, %d1, %acc0 |
    mac.l       %d2, %d3, (%a1)+, %d2, %acc0 |
    subq.l      #1, %d0                 |
    ble.s       20f | loop done         |
10: | loop                              |
    movclr.l    %acc0, %d4              | L = R = l/2 + r/2
    mac.l       %d1, %d3, (%a0)+, %d1, %acc0 |
    mac.l       %d2, %d3, (%a1)+, %d2, %acc0 |
    move.l      %d4, (%a2)+             | output to original buffer
    move.l      %d4, (%a3)+             |
    subq.l      #1, %d0                 |
    bgt.s       10b | loop              |
20: | loop done                         |
    movclr.l    %acc0, %d4              | output last sample
    move.l      %d4, (%a2)              |
    move.l      %d4, (%a3)              |
    movem.l     (%sp), %d2-%d4/%a2-%a3  | restore registers
    lea.l       20(%sp), %sp            | cleanup
    rts                                 |
    .size       channel_mode_proc_mono, .-channel_mode_proc_mono

/****************************************************************************
 * void channel_mode_proc_custom(struct dsp_proc_entry *this,
 *                               struct dsp_buffer **buf_p)
 *
 * Apply stereo width (narrowing/expanding) effect.
 */
    .section    .text
    .align      2
    .global     channel_mode_proc_custom
channel_mode_proc_custom:
    | input: 4(sp) = this, 8(sp) = buf_p
    lea.l       -28(%sp), %sp           | save registers
    movem.l     %d2-%d6/%a2-%a3, (%sp)  |
    movem.l     32(%sp), %a0-%a1        | %a0 = this, %a1 = buf_p
    move.l      (%a1), %a1              | %a1 = buf = *buf_p
    move.l      (%a0), %a2              | %a2 = this->data = &channel_mode_data
    movem.l     (%a1), %d0/%a0-%a1      | %d0 = buf->remcount, %a0 = buf->p32[0],
                                        | %a1 = buf->p32[1]
    movem.l     (%a2), %d3-%d4          | %d3 = sw_gain, %d4 = sw_cross
    move.l      %a0, %a2                | use separate dst pointers since read
    move.l      %a1, %a3                | pointers run one ahead of write
    move.l      (%a0)+, %d1             | prime the input registers
    move.l      (%a1)+, %d2             |
    mac.l       %d1, %d3             , %acc0 |  L = l*gain + r*cross
    mac.l       %d1, %d4, (%a0)+, %d1, %acc1 |  R = r*gain + l*cross
    mac.l       %d2, %d4             , %acc0 |
    mac.l       %d2, %d3, (%a1)+, %d2, %acc1 |
    subq.l      #1, %d0                 |
    ble.b       20f | loop done         |
10: | loop                              |
    movclr.l    %acc0, %d5              |
    movclr.l    %acc1, %d6              |
    mac.l       %d1, %d3             , %acc0 |  L = l*gain + r*cross
    mac.l       %d1, %d4, (%a0)+, %d1, %acc1 |  R = r*gain + l*cross
    mac.l       %d2, %d4             , %acc0 |
    mac.l       %d2, %d3, (%a1)+, %d2, %acc1 |
    move.l      %d5, (%a2)+             |
    move.l      %d6, (%a3)+             |
    subq.l      #1, %d0                 |
    bgt.s       10b | loop              |
20: | loop done                         |
    movclr.l    %acc0, %d5              | output last sample
    movclr.l    %acc1, %d6              |
    move.l      %d5, (%a2)              |
    move.l      %d6, (%a3)              |
    movem.l     (%sp), %d2-%d6/%a2-%a3  | restore registers
    lea.l       28(%sp), %sp            | cleanup
    rts                                 |
    .size       channel_mode_proc_custom, .-channel_mode_proc_custom

/****************************************************************************
 *  void channel_mode_proc_karaoke(struct dsp_proc_entry *this,
 *                                 struct dsp_buffer **buf_p)
 *
 *  Separate channels into side channels.
 */
    .section    .text
    .align      2
    .global     channel_mode_proc_karaoke
channel_mode_proc_karaoke:
    | input: 4(sp) = this, 8(sp) = buf_p
    move.l      8(%sp), %a0             | %a0 = buf_p
    move.l      (%a0), %a0              | %a0 = buf = *buf_p
    lea.l       -20(%sp), %sp           | save registers
    movem.l     %d2-%d4/%a2-%a3, (%sp)  |
    movem.l     (%a0), %d0/%a0-%a1      | %d0 = buf->remcount, %a0 = buf->p32[0],
                                        | %a1 = buf->p32[1]
    move.l      %a0, %a2                | use separate dst pointers since read
    move.l      %a1, %a3                | pointers run one ahead of write
    move.l      #0x40000000, %d3        | %d3 = 0.5
    move.l      (%a0)+, %d1             | prime the input registers
    move.l      (%a1)+, %d2             |
    mac.l       %d1, %d3, (%a0)+, %d1, %acc0 | L = l/2 - r/2
    msac.l      %d2, %d3, (%a1)+, %d2, %acc0 |
    subq.l      #1, %d0                 |
    ble.b       20f | loop done         |
10: | loop                              |
    movclr.l    %acc0, %d4              |
    mac.l       %d1, %d3, (%a0)+, %d1, %acc0 | L = l/2 - r/2
    msac.l      %d2, %d3, (%a1)+, %d2, %acc0 |
    move.l      %d4, (%a2)+             |
    neg.l       %d4                     | R = -L = -(l/2 - r/2) = r/2 - l/2
    move.l      %d4, (%a3)+             |
    subq.l      #1, %d0                 |
    bgt.s       10b | loop              |
20: | loop done                         |
    movclr.l    %acc0, %d4              | output last sample
    move.l      %d4, (%a2)              |
    neg.l       %d4                     | R = -L = -(l/2 - r/2) = r/2 - l/2
    move.l      %d4, (%a3)              |
    movem.l     (%sp), %d2-%d4/%a2-%a3  | restore registers
    lea.l       20(%sp), %sp            | cleanup
    rts                                 |
    .size       channel_mode_proc_karaoke, .-channel_mode_proc_karaoke

/****************************************************************************
 * void filter_process(struct dsp_filter *f, int32_t *buf[], int count,
 *                     unsigned int channels)
 *
 * define HIGH_PRECISION as '1' to make filtering calculate lower bits after
 * shifting. without this, "shift" - 1 of the lower bits will be lost here.
 */
#define HIGH_PRECISION 0
    .text
    .global filter_process
filter_process:
    | input: 4(sp) = f, 8(sp) = buf, 12(sp) = count, 16(sp) = channels
    lea.l       -44(%sp), %sp           | save clobbered regs
#if HIGH_PRECISION
    movem.l     %d2-%d7/%a2-%a6, (%sp)  | .
#else
    movem.l     %d2-%d6/%a2-%a6, (%sp)  |
#endif
    move.l      48(%sp), %a5            | fetch filter structure address
    clr.l       %d6                     | load shift count
    move.b      52(%a5), %d6            | .
    subq.l      #1, %d6                 | EMAC gives us one free shift
#if HIGH_PRECISION
    moveq.l     #8, %d7
    sub.l       %d6, %d7                | shift for lower part of accumulator
#endif
    movem.l     (%a5), %a0-%a4          | load coefs
    lea.l       20(%a5), %a5            | point to filter history

10: | channel loop
    move.l      52(%sp), %a6            | load input channel pointer
    addq.l      #4, 52(%sp)             | point x to next channel
    move.l      (%a6), %a6              |
    move.l      56(%sp), %d5            | number of samples
    movem.l     (%a5), %d0-%d3          | load filter history

    | d0-d3 = history, d4 = temp, d5 = sample count, d6 = upper shift amount,
    | d7 = lower shift amount,a0-a4 = coefs, a5 = history pointer, a6 = buf[ch]
20: | loop
    | Direct form 1 filtering code. We assume DSP has put EMAC in frac mode.
    | y[n] = b0*x[i] + b1*x[i - 1] + b2*x[i - 2] + a1*y[i - 1] + a2*y[i - 2],
    | where y[] is output and x[] is input. This is performed out of order
    | to do parallel load of input value.
    mac.l       %a2, %d1, %acc0         | acc = b2*x[i - 2]
    move.l      %d0, %d1                | fix input history
    mac.l       %a1, %d0, (%a6), %d0, %acc0 | acc += b1*x[i - 1], x[i] -> d0
    mac.l       %a0, %d0, %acc0         | acc += b0*x[i]
    mac.l       %a3, %d2, %acc0         | acc += a1*y[i - 1]
    mac.l       %a4, %d3, %acc0         | acc += a2*y[i - 2]
    move.l      %d2, %d3                | fix output history
#if HIGH_PRECISION
    move.l      %accext01, %d2          | fetch lower part of accumulator
    move.b      %d2, %d4                | clear upper three bytes
    lsr.l       %d7, %d4                | shift lower bits
#endif
    movclr.l    %acc0, %d2              | fetch upper part of result
    asl.l       %d6, %d2                | restore fixed point format
#if HIGH_PRECISION
    or.l        %d2, %d4                | combine lower and upper parts
#endif
    move.l      %d2, (%a6)+             | save result
    subq.l      #1, %d5                 | are we done with this channel?
    bgt         20b | loop

    movem.l     %d0-%d3, (%a5)          | save history back to struct
    lea.l       16(%a5), %a5            | point to next channel's history
    subq.l      #1, 60(%sp)             | have we processed both channels?
    bhi         10b | channel loop

#if HIGH_PRECISION
    movem.l     (%sp), %d2-%d7/%a2-%a6
#else
    movem.l     (%sp), %d2-%d6/%a2-%a6
#endif
    lea.l       44(%sp), %sp
    rts
    .size       filter_process, .-filter_process

/****************************************************************************
 * void sample_output_stereo(struct sample_io_data *this,
 *                           struct dsp_buffer *src,
 *                           struct dsp_buffer *dst)
 *
 * Framework based on the ubiquitous Rockbox line transfer logic for
 * Coldfire CPUs.
 *
 * Does emac clamping and scaling (which proved faster than the usual
 * checks and branches - even single test clamping) and writes using
 * line burst transfers. Also better than writing a single L-R pair per
 * loop but a good deal more code.
 *
 * Attemping bursting during reads is rather futile since the source and
 * destination alignments rarely agree and too much complication will
 * slow us up. The parallel loads seem to do a bit better at least until
 * a pcm buffer can always give line aligned chunk and then aligning the
 * dest can then imply the source is aligned if the source buffers are.
 * For now longword alignment is assumed of both the source and dest.
 *
 */
    .section   .text
    .align      2
    .global    sample_output_stereo
sample_output_stereo:
    | input: 4(sp) = count, 8(sp) = src, 12(sp) = dst
    lea.l       -48(%sp), %sp             | save registers
    move.l      %macsr, %d1               | do it now as at many lines will
    movem.l     %d1-%d7/%a2-%a6, (%sp)    | be the far more common condition
    move.l      #0x80, %macsr             | put emac unit in signed int mode
    movem.l     52(%sp), %a0-%a2          | %a0 = this, %a1 = src, %a2 = dst
    move.l      (%a0), %a0                | %a0 = this->outcount
    move.l      4(%a2), %a4               | %a4 = dst->p16out
    lea.l       (%a4, %a0.l*4), %a0       | %a0 = count -> end address
    movem.l     4(%a1), %a2-%a3           | %a2 = src->p32[0], %a3 = src->p32[1]
    clr.l       %d1                       | %a1 = multiplier: (1 << (16 - scale))
    move.b      19(%a1), %d1              | %d1 = src->format.output_scale
    sub.l       #16, %d1                  |
    neg.l       %d1                       |
    moveq.l     #1, %d0                   |
    asl.l       %d1, %d0                  |
    move.l      %d0, %a1                  |
    move.l      #0x8000, %a6              | %a6 = rounding term
    moveq.l     #28, %d0                  | %d0 = second line bound
    add.l       %a4, %d0                  |
    and.l       #0xfffffff0, %d0          |
    cmp.l       %a0, %d0                  | at least a full line?
    bhi.w       40f | long loop 1 start   | no? do as trailing longwords
    sub.l       #16, %d0                  | %d1 = first line bound
    cmp.l       %a4, %d0                  | any leading longwords?
    bls.b       20f | line loop start     | no? start line loop
10: | long loop 0                         |
    move.l      (%a2)+, %d1               | read longword from L and R
    move.l      %a6, %acc0                |
    move.l      %acc0, %acc1              |
    mac.l       %d1, %a1, (%a3)+, %d2, %acc0 | shift L to high word
    mac.l       %d2, %a1, %acc1           | shift R to high word
    movclr.l    %acc0, %d1                | get possibly saturated results
    movclr.l    %acc1, %d2                |
    swap.w      %d2                       | move R to low word
    move.w      %d2, %d1                  | interleave MS 16 bits of each
    move.l      %d1, (%a4)+               | ...and write both
    cmp.l       %a4, %d0                  |
    bhi.b       10b | long loop 0         |
20: | line loop start                     |
    lea.l       -12(%a0), %a5             | %a5 = at or just before last line bound
30: | line loop                           |
    move.l      (%a3)+, %d4               | get next 4 R samples and scale
    move.l      %a6, %acc0                |
    move.l      %acc0, %acc1              |
    move.l      %acc1, %acc2              |
    move.l      %acc2, %acc3              |
    mac.l       %d4, %a1, (%a3)+, %d5, %acc0 | with saturation
    mac.l       %d5, %a1, (%a3)+, %d6, %acc1 |
    mac.l       %d6, %a1, (%a3)+, %d7, %acc2 |
    mac.l       %d7, %a1, (%a2)+, %d0, %acc3 |
    lea.l       16(%a4), %a4              | increment dest here, mitigate stalls
    movclr.l    %acc0, %d4                | obtain R results
    movclr.l    %acc1, %d5                |
    movclr.l    %acc2, %d6                |
    movclr.l    %acc3, %d7                |
    move.l      %a6, %acc0                |
    move.l      %acc0, %acc1              |
    move.l      %acc1, %acc2              |
    move.l      %acc2, %acc3              |
    mac.l       %d0, %a1, (%a2)+, %d1, %acc0 | get next 4 L samples and scale
    mac.l       %d1, %a1, (%a2)+, %d2, %acc1 | with saturation
    mac.l       %d2, %a1, (%a2)+, %d3, %acc2 |
    mac.l       %d3, %a1             , %acc3 |
    swap.w      %d4                       | a) interleave most significant...
    swap.w      %d5                       |
    swap.w      %d6                       |
    swap.w      %d7                       |
    movclr.l    %acc0, %d0                | obtain L results
    movclr.l    %acc1, %d1                |
    movclr.l    %acc2, %d2                |
    movclr.l    %acc3, %d3                |
    move.w      %d4, %d0                  | a) ... 16 bits of L and R
    move.w      %d5, %d1                  |
    move.w      %d6, %d2                  |
    move.w      %d7, %d3                  |
    movem.l     %d0-%d3, -16(%a4)         | write four stereo samples
    cmp.l       %a4, %a5                  |
    bhi.b       30b | line loop           |
40: | long loop 1 start                   |
    cmp.l       %a4, %a0                  | any longwords left?
    bls.b       60f | output end          | no? stop
50: | long loop 1                         |
    move.l      (%a2)+, %d1               | handle trailing longwords
    move.l      %a6, %acc0                |
    move.l      %acc0, %acc1              |
    mac.l       %d1, %a1, (%a3)+, %d2, %acc0 | the same way as leading ones
    mac.l       %d2, %a1, %acc1           |
    movclr.l    %acc0, %d1                |
    movclr.l    %acc1, %d2                |
    swap.w      %d2                       |
    move.w      %d2, %d1                  |
    move.l      %d1, (%a4)+               |
    cmp.l       %a4, %a0                  |
    bhi.b       50b                       | long loop 1
60: | output end                          |
    movem.l     (%sp), %d1-%d7/%a2-%a6    | restore registers
    move.l      %d1, %macsr               |
    lea.l       48(%sp), %sp              | cleanup
    rts                                   |
    .size      sample_output_stereo, .-sample_output_stereo

/****************************************************************************
 * void sample_output_mono(struct sample_io_data *this,
 *                         struct dsp_buffer *src,
 *                         struct dsp_buffer *dst)
 *
 * Same treatment as sample_output_stereo but for one channel.
 */
    .section   .text
    .align      2
    .global    sample_output_mono
sample_output_mono:
    | input: 4(sp) = count, 8(sp) = src, 12(sp) = dst
    lea.l       -32(%sp), %sp             | save registers
    move.l      %macsr, %d1               | do it now as at many lines will
    movem.l     %d1-%d5/%a2-%a4, (%sp)    | be the far more common condition
    move.l      #0x80, %macsr             | put emac unit in signed int mode
    movem.l     36(%sp), %a0-%a2          | %a0 = this, %a1 = src, %a2 = dst
    move.l      (%a0), %a0                | %a0 = this->outcount
    move.l      4(%a2), %a3               | %a3 = dst->p16out
    movem.l     4(%a1), %a2               | %a2 = src->p32[0]
    lea.l       (%a3, %a0.l*4), %a0       | %a0 = count -> end address
    clr.l       %d1                       | %d5 = multiplier: (1 << (16 - scale))
    move.b      19(%a1), %d1              | %d1 = src->format.output_scale
    sub.l       #16, %d1                  |
    neg.l       %d1                       |
    moveq.l     #1, %d5                   |
    asl.l       %d1, %d5                  |
    move.l      #0x8000, %a4              | %a4 = rounding term
    moveq.l     #28, %d0                  | %d0 = second line bound
    add.l       %a3, %d0                  |
    and.l       #0xfffffff0, %d0          |
    cmp.l       %a0, %d0                  | at least a full line?
    bhi.w       40f | long loop 1 start   | no? do as trailing longwords
    sub.l       #16, %d0                  | %d1 = first line bound
    cmp.l       %a3, %d0                  | any leading longwords?
    bls.b       20f | line loop start     | no? start line loop
10: | long loop 0                         |
    move.l      (%a2)+, %d1               | read longword from L and R
    move.l      %a4, %acc0                |
    mac.l       %d1, %d5, %acc0           | shift L to high word
    movclr.l    %acc0, %d1                | get possibly saturated results
    move.l      %d1, %d2                  |
    swap.w      %d2                       | move R to low word
    move.w      %d2, %d1                  | duplicate single channel into
    move.l      %d1, (%a3)+               | L and R
    cmp.l       %a3, %d0                  |
    bhi.b       10b | long loop 0         |
20: | line loop start                     |
    lea.l       -12(%a0), %a1             | %a1 = at or just before last line bound
30: | line loop                           |
    move.l      (%a2)+, %d0               | get next 4 L samples and scale
    move.l      %a4, %acc0                |
    move.l      %acc0, %acc1              |
    move.l      %acc1, %acc2              |
    move.l      %acc2, %acc3              |
    mac.l       %d0, %d5, (%a2)+, %d1, %acc0 | with saturation
    mac.l       %d1, %d5, (%a2)+, %d2, %acc1 |
    mac.l       %d2, %d5, (%a2)+, %d3, %acc2 |
    mac.l       %d3, %d5             , %acc3 |
    lea.l       16(%a3), %a3              | increment dest here, mitigate stalls
    movclr.l    %acc0, %d0                | obtain results
    movclr.l    %acc1, %d1                |
    movclr.l    %acc2, %d2                |
    movclr.l    %acc3, %d3                |
    move.l      %d0, %d4                  | duplicate single channel
    swap.w      %d4                       | into L and R
    move.w      %d4, %d0                  |
    move.l      %d1, %d4                  |
    swap.w      %d4                       |
    move.w      %d4, %d1                  |
    move.l      %d2, %d4                  |
    swap.w      %d4                       |
    move.w      %d4, %d2                  |
    move.l      %d3, %d4                  |
    swap.w      %d4                       |
    move.w      %d4, %d3                  |
    movem.l     %d0-%d3, -16(%a3)         | write four stereo samples
    cmp.l       %a3, %a1                  |
    bhi.b       30b | line loop           |
40: | long loop 1 start                   |
    cmp.l       %a3, %a0                  | any longwords left?
    bls.b       60f | output end          | no? stop
50: | loop loop 1                         |
    move.l      (%a2)+, %d1               | handle trailing longwords
    move.l      %a4, %acc0                |
    mac.l       %d1, %d5, %acc0           | the same way as leading ones
    movclr.l    %acc0, %d1                |
    move.l      %d1, %d2                  |
    swap.w      %d2                       |
    move.w      %d2, %d1                  |
    move.l      %d1, (%a3)+               |
    cmp.l       %a3, %a0                  |
    bhi.b       50b | long loop 1         |
60: | output end                          |
    movem.l     (%sp), %d1-%d5/%a2-%a4    | restore registers
    move.l      %d1, %macsr               |
    lea.l       32(%sp), %sp              | cleanup
    rts                                   |
    .size      sample_output_mono, .-sample_output_mono
