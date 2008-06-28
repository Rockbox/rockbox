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

/****************************************************************************
 * void dsp_apply_gain(int count, struct dsp_data *data, int32_t *buf[])
 */
    .section    .text
    .align      2
    .global     dsp_apply_gain
dsp_apply_gain:
    lea.l       -20(%sp), %sp           | save registers
    movem.l     %d2-%d4/%a2-%a3, (%sp)  |
    movem.l     28(%sp), %a0-%a1        | %a0 = data,
                                        | %a1 = buf
    move.l      4(%a0), %d1             | %d1 = data->num_channels
    move.l      32(%a0), %a0            | %a0 = data->gain (in s8.23)
10: | channel loop                      |
    move.l      24(%sp), %d0            | %d0 = count
    move.l      -4(%a1, %d1.l*4), %a2   | %a2 = s = buf[ch-1]
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
 * void apply_crossfeed(int count, int32_t *buf[])
 */
    .section    .text
    .align      2
    .global     apply_crossfeed 
apply_crossfeed:
    lea.l       -44(%sp), %sp           |
    movem.l     %d2-%d7/%a2-%a6, (%sp)  | save all regs
    movem.l     48(%sp), %d7/%a4        | %d7 = count, %a4 = src
    movem.l     (%a4), %a4-%a5          | %a4 = src[0], %a5 = src[1]
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
    .word       0x51fb | tpf.l          | trap the buffer wrap
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
 * int dsp_downsample(int count, struct dsp_data *data,
 *                    in32_t *src[], int32_t *dst[])
 */
    .section    .text
    .align      2
    .global     dsp_downsample
dsp_downsample:
    lea.l       -40(%sp), %sp           | save non-clobberables
    movem.l     %d2-%d7/%a2-%a5, (%sp)  |
    movem.l     44(%sp), %d2/%a0-%a2    | %d2 = count
                                        | %a0 = data
                                        | %a1 = src
                                        | %a2 = dst
    movem.l     4(%a0), %d3-%d4         | %d3 = ch = data->num_channels
                                        | %d4 = delta = data->resample_data.delta
    moveq.l     #16, %d7                | %d7 = shift
10: | channel loop                      |
    move.l      12(%a0), %d5            | %d5 = phase = data->resample_data.phase
    move.l      -4(%a1, %d3.l*4), %a3   | %a3 = s = src[ch-1]
    move.l      -4(%a2, %d3.l*4), %a4   | %a4 = d = dst[ch-1]
    lea.l       12(%a0, %d3.l*4), %a5   | %a5 = &data->resample_data.ast_sample[ch-1]
    move.l      (%a5), %d0              | %d0 = last = data->resample_data.last_sample[ch-1]
    move.l      -4(%a3, %d2.l*4), (%a5) | data->resample_data.last_sample[ch-1] = s[count-1]
    move.l      %d5, %d6                | %d6 = pos = phase >> 16
    lsr.l       %d7, %d6                |
    cmp.l       %d2, %d6                | past end of samples?
    bge.b       40f | skip resample loop| yes? skip loop
    tst.l       %d6                     | need last sample of prev. frame?
    bne.b       20f | resample loop     | no? start main loop
    move.l      (%a3, %d6.l*4), %d1     | %d1 = s[pos]
    bra.b       30f | resample start last | start with last (last in %d0)
20: | resample loop                     |
    lea.l       -4(%a3, %d6.l*4), %a5   | load s[pos-1] and s[pos]
    movem.l     (%a5), %d0-%d1          |
30: | resample start last               |
    sub.l       %d0, %d1                | %d1 = diff = s[pos] - s[pos-1]
    move.l      %d0, %acc0              | %acc0 = previous sample
    move.l      %d5, %d0                | frac = (phase << 16) >> 1
    lsl.l       %d7, %d0                |
    lsr.l       #1, %d0                 |
    mac.l       %d0, %d1, %acc0         | %acc0 += frac * diff
    add.l       %d4, %d5                | phase += delta
    move.l      %d5, %d6                | pos = phase >> 16
    lsr.l       %d7, %d6                |
    movclr.l    %acc0, %d0              |
    move.l      %d0, (%a4)+             | *d++ = %d0
    cmp.l       %d2, %d6                | pos < count?
    blt.b       20b | resample loop     | yes? continue resampling
40: | skip resample loop                |
    subq.l      #1, %d3                 | ch > 0?
    bgt.b       10b | channel loop      | yes? process next channel
    lsl.l       %d7, %d2                | wrap phase to start of next frame
    sub.l       %d2, %d5                | data->resample_data.phase =
    move.l      %d5, 12(%a0)            | ... phase - (count << 16)
    move.l      %a4, %d0                | return d - d[0]
    sub.l       (%a2), %d0              |
    asr.l       #2, %d0                 | convert bytes->samples
    movem.l     (%sp), %d2-%d7/%a2-%a5  | restore non-clobberables
    lea.l       40(%sp), %sp            | cleanup stack
    rts                                 | buh-bye
    .size       dsp_downsample,.-dsp_downsample

/****************************************************************************
 * int dsp_upsample(int count, struct dsp_data *dsp,
 *                  int32_t *src[], int32_t *dst[])
 */
    .section    .text
    .align      2
    .global     dsp_upsample
dsp_upsample:
    lea.l       -40(%sp), %sp           | save non-clobberables
    movem.l     %d2-%d7/%a2-%a5, (%sp)  |
    movem.l     44(%sp), %d2/%a0-%a2    | %d2 = count
                                        | %a0 = data
                                        | %a1 = src
                                        | %a2 = dst
    movem.l      4(%a0), %d3-%d4        | %d3 = ch = channels
                                        | %d4 = delta = data->resample_data.delta
    swap        %d4                     | swap delta to high word to use...
                                        | ...carries to increment position
10: | channel loop                      |
    move.l      12(%a0), %d5            | %d5 = phase = data->resample_data.phase
    move.l      -4(%a1, %d3.l*4), %a3   | %a3 = s = src[ch-1]
    lea.l       12(%a0, %d3.l*4), %a4   | %a4 = &data->resample_data.last_sample[ch-1]
    lea.l       -4(%a3, %d2.l*4), %a5   | %a5 = src_end = &src[count-1]
    move.l      (%a4), %d0              | %d0 = last = data->resample_data.last_sample[ch-1]
    move.l      (%a5), (%a4)            | data->resample_data.last_sample[ch-1] = s[count-1]
    move.l      -4(%a2, %d3.l*4), %a4   | %a4 = d = dst[ch-1]
    move.l      (%a3)+, %d1             | fetch first sample - might throw this...
                                        | ...away later but we'll be preincremented
    move.l      %d1, %d6                | save sample value
    sub.l       %d0, %d1                | %d1 = diff = s[0] - last
    swap        %d5                     | swap phase to high word to use
                                        | carries to increment position
    move.l      %d5, %d7                | %d7 = pos = phase >> 16
    clr.w       %d5                     |
    eor.l       %d5, %d7                | pos == 0?
    beq.b       40f | loop start        | yes? start loop
    cmp.l       %d2, %d7                | past end of samples?
    bge.b       50f | skip resample loop| yes? go to next channel and collect info
    lea.l       (%a3, %d7.l*4), %a3     | %a3 = s = &s[pos+1]
    movem.l     -8(%a3), %d0-%d1        | %d0 = s[pos-1], %d1 = s[pos]
    move.l      %d1, %d6                | save sample value
    sub.l       %d0, %d1                | %d1 = diff = s[pos] - s[pos-1]
    bra.b       40f | loop start        |
20: | next sample loop                  |
    move.l      %d6, %d0                | move previous sample to %d0
    move.l      (%a3)+, %d1             | fetch next sample
    move.l      %d1, %d6                | save sample value
    sub.l       %d0, %d1                | %d1 = diff = s[pos] - s[pos-1]
30: | same sample loop                  |
    movclr.l    %acc0, %d7              | %d7 = result
    move.l      %d7, (%a4)+             | *d++ = %d7
40: | loop start                        |
    lsr.l       #1, %d5                 | make phase into frac
    move.l      %d0, %acc0              | %acc0 = s[pos-1]
    mac.l       %d1, %d5, %acc0         | %acc0 = diff * frac
    lsl.l       #1, %d5                 | restore frac to phase
    add.l       %d4, %d5                | phase += delta
    bcc.b       30b | same sample loop  | load next values?
    cmp.l       %a5, %a3                | src <= src_end?
    bls.b       20b | next sample loop  | yes? continue resampling
    movclr.l    %acc0, %d7              | %d7 = result
    move.l      %d7, (%a4)+             | *d++ = %d7
50: | skip resample loop                |
    subq.l      #1, %d3                 | ch > 0?
    bgt.b       10b | channel loop      | yes? process next channel
    swap        %d5                     | wrap phase to start of next frame
    move.l      %d5, 12(%a0)            | ...and save in data->resample_data.phase
    move.l      %a4, %d0                | return d - d[0]
    sub.l       (%a2), %d0              |
    movem.l     (%sp), %d2-%d7/%a2-%a5  | restore non-clobberables
    asr.l       #2, %d0                 | convert bytes->samples
    lea.l       40(%sp), %sp            | cleanup stack
    rts                                 | buh-bye
    .size       dsp_upsample,.-dsp_upsample

/****************************************************************************
 * void channels_process_sound_chan_mono(int count, int32_t *buf[])
 *
 * Mix left and right channels 50/50 into a center channel.
 */
    .section    .text
    .align      2
    .global     channels_process_sound_chan_mono
channels_process_sound_chan_mono:
    movem.l     4(%sp), %d0/%a0         | %d0 = count, %a0 = buf
    lea.l       -20(%sp), %sp           | save registers
    movem.l     %d2-%d4/%a2-%a3, (%sp)  |
    movem.l     (%a0), %a0-%a1          | get channel pointers
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
 * void channels_process_sound_chan_custom(int count, int32_t *buf[])
 *
 * Apply stereo width (narrowing/expanding) effect.
 */
    .section    .text
    .align      2
    .global     channels_process_sound_chan_custom
channels_process_sound_chan_custom:
    movem.l     4(%sp), %d0/%a0         | %d0 = count, %a0 = buf
    lea.l       -28(%sp), %sp           | save registers
    movem.l     %d2-%d6/%a2-%a3, (%sp)  |
    movem.l     (%a0), %a0-%a1          | get channel pointers
    move.l      %a0, %a2                | use separate dst pointers since read
    move.l      %a1, %a3                | pointers run one ahead of write
    move.l      dsp_sw_gain, %d3        | load straight (mid) gain
    move.l      dsp_sw_cross, %d4       | load cross (side) gain
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
 *  void channels_process_sound_chan_karaoke(int count, int32_t *buf[])
 *
 *  Separate channels into side channels.
 */
    .section    .text
    .align      2
    .global     channels_process_sound_chan_karaoke
channels_process_sound_chan_karaoke:
    movem.l     4(%sp), %d0/%a0         | %d0 = count, %a0 = buf
    lea.l       -20(%sp), %sp           | save registers
    movem.l     %d2-%d4/%a2-%a3, (%sp)  |
    movem.l     (%a0), %a0-%a1          | get channel src pointers
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
 *                               int32_t *src[], int16_t *dst)
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
 *                         int32_t *src[], int16_t *dst)
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
