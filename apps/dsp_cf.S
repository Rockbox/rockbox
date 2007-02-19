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

/****************************************************************************
 * apply_crossfeed(int32_t* src[], int count)
 */
    .section    .text
    .global     apply_crossfeed 
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
    jlt .cfnowrap
    moveq.l #13*8, %d4
    sub.l %d4, %a0                      | wrap back delay line ptr as well
    clr.l %d4
.cfnowrap:
    subq.l #1, %d7
    jne .cfloop
    | save data back to struct
    lea.l crossfeed_data + 4*4, %a1
    movem.l %d0-%d3, (%a1)
    move.l %d4, (30*4, %a1)
    movem.l (%sp), %d2-%d7/%a2-%a6
    lea.l (44, %sp), %sp
    rts
.cfend:
    .size       apply_crossfeed,.cfend-apply_crossfeed

/****************************************************************************
 * dsp_downsample(int channels, int count, struct resample_data *r,
 *                in32_t **src, int32_t **dst)
 */
    .section    .text
    .global     dsp_downsample
dsp_downsample:
    lea.l       -40(%sp), %sp           | save non-clobberables
    movem.l     %d2-%d7/%a2-%a5, (%sp)  |
    movem.l     44(%sp), %d2-%d3/%a0-%a2| %d2 = ch = channels
                                        | %d3 = count
                                        | %a0 = r
                                        | %a1 = src
                                        | %a2 = dst
    move.l      4(%a0), %d4             | %d4 = delta = r->delta
    move.l      #16, %d7                | %d7 = shift
.dschannel_loop:
    move.l      (%a0), %d5              | %d5 = phase = r->phase
    move.l      -4(%a1, %d2.l*4), %a3   | %a3 = s = src[ch-1]
    move.l      -4(%a2, %d2.l*4), %a4   | %a4 = d = dst[ch-1]
    lea.l       4(%a0, %d2.l*4), %a5    | %a5 = &r->last_sample[ch-1]
    move.l      (%a5), %d0              | %d0 = last = r->last_sample[ch-1]
    move.l      -4(%a3, %d3.l*4), %d1   | r->last_sample[ch-1] = s[count-1]
    move.l      %d1, (%a5)              |
    move.l      %d5, %d6                | %d6 = pos = phase >> 16
    lsr.l       %d7, %d6                |
    cmp.l       %d3, %d6                | past end of samples?
    bge.b       .dsloop_skip            | yes? skip loop
    tst.l       %d6                     | need last sample of prev. frame?
    bne.b       .dsloop                 | no? start main loop
    move.l      (%a3, %d6.l*4), %d1     | %d1 = s[pos]
    bra.b       .dsuse_last_start       | start with last (last in %d0)
.dsloop:
    lea.l       -4(%a3, %d6.l*4), %a5   | load s[pos-1] and s[pos]
    movem.l     (%a5), %d0-%d1          |
.dsuse_last_start:
    sub.l       %d0, %d1                | %d1 = diff = s[pos] - s[pos-1]
    move.l      %d0, %acc0              | %acc0 = previous sample
    move.l      %d5, %d0                | frac = (phase << 16) >> 1
    lsl.l       %d7, %d0                |
    lsr.l       #1, %d0                 |
    mac.l       %d0, %d1, %acc0         | %acc0 += frac * diff
    move.l      %acc0, %d0              |
    add.l       %d4, %d5                | phase += delta
    move.l      %d5, %d6                | pos = phase >> 16
    lsr.l       %d7, %d6                |
    move.l      %d0, (%a4)+             | *d++ = %d0
    cmp.l       %d3, %d6                | pos < count?
    blt.b       .dsloop                 | yes? continue resampling
.dsloop_skip:
    subq.l      #1, %d2                 | ch > 0?
    bgt.b       .dschannel_loop         | yes? process next channel
    asl.l       %d7, %d3                | wrap phase to start of next frame
    sub.l       %d3, %d5                | r->phase = phase - (count << 16)
    move.l      %d5, (%a0)              |
    move.l      %a4, %d0                | return d - d[0]
    sub.l       (%a2), %d0              |
    asr.l       #2, %d0                 | convert bytes->samples
    movem.l     (%sp), %d2-%d7/%a2-%a5  | restore non-clobberables
    move.l      %acc1, %acc0            | clear %acc0
    lea.l       40(%sp), %sp            | cleanup stack
    rts                                 | buh-bye
.dsend:
    .size       dsp_downsample,.dsend-dsp_downsample

/****************************************************************************
 * dsp_upsample(int channels, int count, struct resample_data *r,
 *              in32_t **src, int32_t **dst)
 */
    .section    .text
    .global     dsp_upsample
dsp_upsample:
    lea.l       -40(%sp), %sp           | save non-clobberables
    movem.l     %d2-%d7/%a2-%a5, (%sp)  |
    movem.l     44(%sp), %d2-%d3/%a0-%a2| %d2 = ch = channels
                                        | %d3 = count
                                        | %a0 = r
                                        | %a1 = src
                                        | %a2 = dst
    move.l      4(%a0), %d4             | %d4 = delta = r->delta
    swap        %d4                     | swap delta to high word to use
                                        | carries to increment position
.uschannel_loop:
    move.l      (%a0), %d5              | %d5 = phase = r->phase
    move.l      -4(%a1, %d2.l*4), %a3   | %a3 = s = src[ch-1]
    move.l      -4(%a2, %d2.l*4), %a4   | %a4 = d = dst[ch-1]
    lea.l       4(%a0, %d2.l*4), %a5    | %a5 = &r->last_sample[ch-1]
    move.l      (%a5), %d0              | %d0 = last = r->last_sample[ch-1]
    move.l      -4(%a3, %d3.l*4), %d1   | r->last_sample[ch-1] = s[count-1]
    move.l      %d1, (%a5)              |
    moveq.l     #16, %d1                | %d0 = shift
    move.l      %d5, %d6                | %d6 = pos = phase >> 16
    lsl.l       %d1, %d5                | swap phase to high word to use
                                        | carries to increment position
    lsr.l       %d1, %d6                | pos == 0?
    bne.b       .usstart_1              | no? transistion from down
    move.l      (%a3), %d1              | %d1 = s[0]
    sub.l       %d0, %d1                | diff = s[pos] - last
    bra.b       .usloop_0               | jump to typical start point
.usstart_1:
    cmp.l       %d3, %d6                | past end of samples?
    bge.b       .usloop_skip            | yes? skip loop
.usloop_1:
    lea.l       -4(%a3, %d6.l*4), %a5   | load s[pos-1] and s[pos]
    movem.l     (%a5), %d0-%d1          |
    sub.l       %d0, %d1                | %d1 = diff = s[pos] - s[pos-1]
.usloop_0:
    move.l      %d0, %acc0              | %acc0 = previous sample
    lsr.l       #1, %d5                 | make phase into frac
    mac.l       %d1, %d5, %acc0         | %acc0 += diff * frac
    move.l      %acc0, %d7              |
    lsl.l       #1, %d5                 | restore frac to phase
    move.l      %d7, (%a4)+             | *d++ = %d0
    add.l       %d4, %d5                | phase += delta
    bcc.b       .usloop_0               | load next values?
    addq.l      #1, %d6                 | increment position
    cmp.l       %d3, %d6                | pos < count?
    blt.b       .usloop_1               | yes? continue resampling
.usloop_skip:
    subq.l      #1, %d2                 | ch > 0?
    bgt.b       .uschannel_loop         | yes? process next channel
    swap        %d5                     | wrap phase to start of next frame
    move.l      %d5, (%a0)              | ...and save in r->phase
    move.l      %a4, %d0                | return d - d[0]
    sub.l       (%a2), %d0              |
    asr.l       #2, %d0                 | convert bytes->samples
    movem.l     (%sp), %d2-%d7/%a2-%a5  | restore non-clobberables
    move.l      %acc1, %acc0            | clear %acc0
    lea.l       40(%sp), %sp            | cleanup stack
    rts                                 | buh-bye
.usend:
    .size       dsp_upsample,.usend-dsp_upsample
