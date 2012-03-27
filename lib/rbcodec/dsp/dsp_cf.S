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
 * Portions Copyright (C) 2007 Michael Sevakis
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
 * void dsp_apply_gain(struct dsp_data *data,
 *                     struct dsp_buffer **buf_p)
 */
    .section    .text
    .align      2
    .global     dsp_apply_gain
dsp_apply_gain:
    movem.l     4(%sp), %a0-%a1         | %a0 = data,
                                        | %a1 = buf_p
    move.l      (%a1), %a1              | %a1 = buf = *buf_p
    lea.l       -20(%sp), %sp           | save registers
    movem.l     %d2-%d4/%a2-%a3, (%sp)  |
    move.l      4(%a0), %d1             | %d1 = data->num_channels
    move.l      24(%a0), %a0            | %a0 = data->gain (in s8.23)
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
    .size       dsp_apply_gain,.-dsp_apply_gain

/****************************************************************************
 * void apply_crossfeed(struct dsp_data *data,
 *                      struct dsp_buffer **buf_p)
 */
    .section    .text
    .align      2
    .global     apply_crossfeed
apply_crossfeed:
    lea.l       -44(%sp), %sp           |
    movem.l     %d2-%d7/%a2-%a6, (%sp)  | save all regs
    move.l      52(%sp), %a4            | %a4 = buf_p
    move.l      (%a4), %a4              | %a4 = buf = *buf_p
    movem.l     (%a4), %d7/%a4-%a5      | %d7 = buf->remcount, %a4 = buf->p32[0],
                                        | %a5 = buf->p32[1]
    lea.l       crossfeed_data, %a1     | %a1 = &crossfeed_data
    move.l      (%a1)+, %d6             | %d6 = direct gain
    movem.l     12(%a1), %d0-%d3        | fetch filter history samples
    move.l      132(%a1), %a0           | fetch delay line address
    movem.l     (%a1), %a1-%a3          | load filter coefs
    lea.l       crossfeed_data+136, %a6 | %a6 = delay line wrap limit
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
    lea.l       -104(%a0), %a0          | wrap it up
    subq.l      #1, %d7                 | --count > 0 ?
    bgt.b       10b | loop              | yes? do more
    movclr.l    %acc0, %d4              | write last outputs
    move.l      %d4, (%a4)              | .
    movclr.l    %acc1, %d5              | .
    move.l      %d5, (%a5)              | .
    lea.l       crossfeed_data+16, %a1  | save data back to struct
    movem.l     %d0-%d3, (%a1)          | ...history
    move.l      %a0, 120(%a1)           | ...delay_p
    movem.l     (%sp), %d2-%d7/%a2-%a6  | restore all regs
    lea.l       44(%sp), %sp            |
    rts                                 |
    .size       apply_crossfeed,.-apply_crossfeed

/****************************************************************************
 * int dsp_lin_resample(struct dsp_data *data, struct dsp_buffer *src,
 *                      struct dsp_buffer *dst)
 */
    .section    .text
    .align      2
    .global     dsp_lin_resample
dsp_lin_resample:
    lea.l       -44(%sp), %sp           | save non-volatiles
    movem.l     %d2-%d7/%a2-%a6, (%sp)  |
    movem.l     48(%sp), %a0-%a2        | %a0 = data
                                        | %a1 = src
                                        | %a2 = dst
    move.l      4(%a0), %d1             | %d1 = ch = data->num_channels
    moveq.l     #16, %d7                | %d7 = shift
10: | channel loop                      |
    movem.l     8(%a0), %d2-%d3         | %d2 = delta = data->resample_data.delta,
                                        | %d3 = phase = data->resample_data.phase
    move.l      (%a1), %d4              | %d4 = srcrem = src->remcount
    move.l      12(%a2), %d5            | %d5 = dstrem = dst->bufcount
    move.l      %d3, %d0                | %d0 = pos
    lsr.l       %d7, %d0                | ...
    move.l      (%a1, %d1.l*4), %a3     | %a3 = s = src->p32[ch]
    move.l      (%a2, %d1.l*4), %a4     | %a4 = d = dst->p32[ch]
    cmp.l       %d4, %d0                | pos = MIN(pos, srcrem)
    blt.b       11f                     |
    move.l      %d4, %d0                | pos = srcrem
    move.l      -4(%a3, %d0.l*4), %d6   | %d6 = last = s[pos - 1]
    bra.w       40f | channel done      |
11:                                     |
    tst.l       %d0                     | pos > 0?
    bgt.b       12f                     |
    move.l      12(%a0, %d1.l*4), %d6   | %d6 = last = last_sample[ch]
    tpf.l                               | trap next move.l
12:                                     |
    move.l      -4(%a3, %d0.l*4), %d6   | %d6 = last = s[pos - 1]
    cmp.l       #0x10000, %d2           | delta >= 1.0?
    bhs.w       30f | downsample        | yes? downsampling
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
    bra.b       22f | usloop start      |
    /* Funky loop structure is to avoid emac latency stalls */
20: | next sample loop                  |
    move.l      (%a3)+, %a5             | %a5 = s[pos]
    move.l      %a5, %a6                | %a6 = diff = s[pos] - last
    sub.l       %d6, %a6                |
21: | same sample loop                  |
    movclr.l    %acc0, %d7              | *d++ = %d7 = result
    move.l      %d7, (%a4)+             |
22: | usloop start                      |
    move.l      %d6, %acc0              | %acc0 = last
    mac.l       %d3, %a6, %acc0         | %acc0 += frac * diff
    subq.l      #1, %d5                 | dstrem <= 0?
    ble.b       23f | dest full         | yes? stop
    add.l       %d2, %d3                | phase += delta
    bpl.b       21b | same sample loop  | load next values?
    move.l      %a5, %d6                |
    bclr.l      #31, %d3                | clear sign bit
    addq.l      #1, %d0                 | dte > 0?
    bmi.b       20b | next sample loop  | yes? continue resampling
    tpf.w                               | trap next add.l
23: | dest full                         |
    add.l       %d2, %d3                | do phase advance
    lsl.l       #1, %d3                 | frac -> phase
    bcc.b       24f | us still same     |
    move.l      %a5, %d6                | moved to new s[pos]
    addq.l      #1, %d0                 |
24: | us still same                     |
    movclr.l    %acc0, %d7              | *d = %d7 = result
    move.l      %d7, (%a4)              |
    add.l       %d4, %d0                | %d0 = -dte + srcrem = pos
    or.l        %d0, %d3                | restore phase
    swap        %d3                     |
    moveq.l     #16, %d7                | %d7 = shift
    bra.b       40f | channel done      |
                                        |
    /** Downsampling **/                |
30: | downsample                        |
    move.l      (%a3, %d0.l*4), %a5     | %a5 = s[pos]
    bra.b       32f | dsloop start      |
31: | dsloop                            |
    lea.l       -4(%a3, %d0.l*4), %a5   | %d6 = s[pos - 1], %a5 = s[pos]
    movem.l     (%a5), %d6/%a5          |
32: | dsloop start                      |
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
    ble.b       33f | dst full          | yes? stop
    cmp.l       %d4, %d0                | pos < srcrem?
    blt.b       31b | dsloop            | yes? continue resampling
    tpf.l                               | trap cmp.l and ble.b
33: | dst full                          |
    cmp.l       %d4, %d0                | pos = MIN(pos, srcrem)
    ble.b       34f                     |
    move.l      %d4, %d0                |
34: |                                   |
    move.l      -4(%a3, %d0.l*4), %d6   | %d6 = s[pos - 1]
                                        |
40: | channel done                      |
    move.l      %d6, 12(%a0, %d1.l*4)   | last_sample[ch] = last
    subq.l      #1, %d1                 | ch > 0?
    bgt.w       10b | channel loop      | yes? process next channel
                                        |
    move.l      12(%a2), %d1            | %d1 = dst->bufcount
    sub.l       %d5, %d1                | written = dst->bufcount - dstrem
    move.l      %d1, (%a2)              | dst->remcount = written
    move.l      %d0, %d1                | wrap phase to position in next frame
    lsl.l       %d7, %d1                | resample_data.phase = phase - (pos << 16)
    sub.l       %d1, %d3                | ...
    move.l      %d3, 12(%a0)            | ...
    movem.l     (%sp), %d2-%d7/%a2-%a6  | restore non-volatiles
    lea.l       44(%sp), %sp            | cleanup stack
    rts                                 | buh-bye

    .size       dsp_lin_resample,.-dsp_lin_resample


/****************************************************************************
 * void channels_process_sound_chan_mono(struct dsp_data *data,
 *                                       struct dsp_buffer **buf_p)
 *
 * Mix left and right channels 50/50 into a center channel.
 */
    .section    .text
    .align      2
    .global     channels_process_sound_chan_mono
channels_process_sound_chan_mono:
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
    .size       channels_process_sound_chan_mono, \
                .-channels_process_sound_chan_mono

/****************************************************************************
 * void channels_process_sound_chan_custom(struct dsp_data *data,
 *                                         struct dsp_buffer **buf_p)
 *
 * Apply stereo width (narrowing/expanding) effect.
 */
    .section    .text
    .align      2
    .global     channels_process_sound_chan_custom
channels_process_sound_chan_custom:
    move.l      8(%sp), %a0             | %a0 = buf_p
    move.l      (%a0), %a0              | %a0 = buf = *buf_p
    lea.l       -28(%sp), %sp           | save registers
    movem.l     %d2-%d6/%a2-%a3, (%sp)  |
    movem.l     (%a0), %d0/%a0-%a1      | %d0 = buf->remcount, %a0 = buf->p32[0],
                                        | %a1 = buf->p32[1]
    lea.l       channels_mode_data, %a2 | %a2 = &dsp_channels_data
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
    .size       channels_process_sound_chan_custom, \
                .-channels_process_sound_chan_custom

/****************************************************************************
 *  void channels_process_sound_chan_karaoke(struct dsp_data *data,
 *                                           struct dsp_buffer **buf_p)
 *
 *  Separate channels into side channels.
 */
    .section    .text
    .align      2
    .global     channels_process_sound_chan_karaoke
channels_process_sound_chan_karaoke:
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
    .size       channels_process_sound_chan_karaoke, \
                .-channels_process_sound_chan_karaoke

/****************************************************************************
 * void sample_output_stereo(int count, struct dsp_data *data,
 *                           int32_t const * const src[], int16_t *dst)
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
    lea.l       -48(%sp), %sp             | save registers
    move.l      %macsr, %d1               | do it now as at many lines will
    movem.l     %d1-%d7/%a2-%a6, (%sp)    | be the far more common condition
    move.l      #0x80, %macsr             | put emac unit in signed int mode
    movem.l     52(%sp), %a0-%a2/%a4      |
    lea.l       (%a4, %a0.l*4), %a0       | %a0 = end address
    move.l      (%a1), %d1                | %a1 = multiplier: (1 << (16 - scale))
    sub.l       #16, %d1                  |
    neg.l       %d1                       |
    moveq.l     #1, %d0                   |
    asl.l       %d1, %d0                  |
    move.l      %d0, %a1                  |
    move.l      #0x8000, %a6              | %a6 = rounding term
    movem.l     (%a2), %a2-%a3            | get L/R channel pointers
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
    swap        %d2                       | move R to low word
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
    swap        %d4                       | a) interleave most significant...
    swap        %d5                       |
    swap        %d6                       |
    swap        %d7                       |
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
    swap        %d2                       |
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
 * void sample_output_mono(int count, struct dsp_data *data,
 *                         int32_t const * const src[], int16_t *dst)
 *
 * Same treatment as sample_output_stereo but for one channel.
 */
    .section   .text
    .align      2
    .global    sample_output_mono
sample_output_mono:
    lea.l       -32(%sp), %sp             | save registers
    move.l      %macsr, %d1               | do it now as at many lines will
    movem.l     %d1-%d5/%a2-%a4, (%sp)    | be the far more common condition
    move.l      #0x80, %macsr             | put emac unit in signed int mode
    movem.l     36(%sp), %a0-%a3          |
    lea.l       (%a3, %a0.l*4), %a0       | %a0 = end address
    move.l      (%a1), %d1                | %d5 = multiplier: (1 << (16 - scale))
    sub.l       #16, %d1                  |
    neg.l       %d1                       |
    moveq.l     #1, %d5                   |
    asl.l       %d1, %d5                  |
    move.l      #0x8000, %a4              | %a4 = rounding term
    movem.l     (%a2), %a2                | get source channel pointer
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
    swap        %d2                       | move R to low word
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
    swap        %d4                       | into L and R
    move.w      %d4, %d0                  |
    move.l      %d1, %d4                  |
    swap        %d4                       |
    move.w      %d4, %d1                  |
    move.l      %d2, %d4                  |
    swap        %d4                       |
    move.w      %d4, %d2                  |
    move.l      %d3, %d4                  |
    swap        %d4                       |
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
    swap        %d2                       |
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
