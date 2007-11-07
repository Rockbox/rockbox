/* Copyright (C) 2007 Thom Johansen */
/**
   @file filters_cf.S
   @brief Various analysis/synthesis filters (Coldfire version)
*/
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

    .text
/* void iir_mem16(const spx_word16_t *x, const spx_coef_t *den, spx_word16_t *y, int N, int ord, spx_mem_t *mem, char *stack) */
    .global iir_mem16
iir_mem16:
    lea.l    (-44, %sp), %sp
    movem.l  %d2-%d7/%a2-%a6, (%sp)
    movem.l  (44+4, %sp), %a3-%a5   | a3 = x, a4 = den, a5 = y
    movem.l  (44+20, %sp), %d0/%a6  | d0 = ord, a6 = mem
    moveq.l  #8, %d1                | Jump to correct routine based on 'ord'
    cmp.l    %d1, %d0
    jeq      .order_8
    moveq.l  #10, %d1
    cmp.l    %d1, %d0
    jeq      .order_10
    jra      .exit

    | TODO: try using direct form 1 filtering
    | d0 = y[i], d1-d7, a0 = mem[0] .. mem[7]
    | a3 = x, a4 = den, a5 = y, a6 = temp 
.order_8:
    movem.l  (%a6), %d1-%d7/%a0 | Fetch mem[] array
0:
    moveq.l  #13, %d0
    add.l    #4096, %d1
    asr.l    %d0, %d1           | mem[0] >> 13 with rounding
    move.w   (%a3)+, %d0
    ext.l    %d0
    add.l    %d1, %d0           | Add with x[i]
    move.l   #32767, %d1
    move.l   #65534, %a6
    add.l    %d1, %d0           | Bias result to [-1..65534]
    cmp.l    %a6, %d0           | Now do clip to [0..65534] range
    jls      2f
    jpl      1f
    clr.l    %d0                | Clip low
    .word    0x51fa             | trapf.w, shadow next insn
1:
    move.l   %a6, %d0           | Clip high
2:
    sub.l    %d1, %d0           | Bias clipped result back to [-32767..32767]
    move.w   %d0, (%a5)+        | Write result to y[i]
    neg.l    %d0                | msac.w is bugged in gas, do this for now
    move.l   (%a4)+, %a6        | Fetch den[0] and den[1]
    mac.w    %a6u, %d0l, %acc0
    mac.w    %a6l, %d0l, (%a4)+, %a6, %acc1
    mac.w    %a6u, %d0l, %acc2
    mac.w    %a6l, %d0l, (%a4)+, %a6, %acc3
    movclr.l %acc0, %d1
    add.l    %d2, %d1           | mem[0] = mem[1] - den[0]*y[i]
    movclr.l %acc1, %d2
    add.l    %d3, %d2           | mem[1] = mem[2] - den[1]*y[i]
    movclr.l %acc2, %d3
    add.l    %d4, %d3           | mem[2] = mem[3] - den[2]*y[i]
    movclr.l %acc3, %d4
    add.l    %d5, %d4           | mem[3] = mem[4] - den[3]*y[i]
    mac.w    %a6u, %d0l, %acc0
    mac.w    %a6l, %d0l, (%a4)+, %a6, %acc1
    mac.w    %a6u, %d0l, %acc2
    mac.w    %a6l, %d0l, %acc3
    lea.l    (-16, %a4), %a4    | wrap den pointer back to den[0]
    movclr.l %acc0, %d5
    add.l    %d6, %d5           | mem[4] = mem[5] - den[4]*y[i]
    movclr.l %acc1, %d6
    add.l    %d7, %d6           | mem[5] = mem[6] - den[5]*y[i]
    movclr.l %acc2, %d7
    add.l    %a0, %d7           | mem[6] = mem[7] - den[6]*y[i]
    movclr.l %acc3, %a0         | mem[7] = -den[7]*y[i]
    subq.l   #1, (44+16, %sp)   | Have we done all samples?
    jne      0b
    move.l   (44+24, %sp), %a6  | Fetch mem pointer
    movem.l  %d1-%d7/%a0, (%a6) | Save back mem[]
    jra     .exit

    | d0 = y[i], d1-d7, a0-a2 = mem[0] .. mem[9]
    | a3 = x, a4 = den, a5 = y, a6 = temp 
.order_10:
    movem.l  (%a6), %d1-%d7/%a0-%a2 | Fetch mem[] array 
0:
    moveq.l  #13, %d0
    add.l    #4096, %d1
    asr.l    %d0, %d1           | mem[0] >> 13 with rounding
    move.w   (%a3)+, %d0
    ext.l    %d0
    add.l    %d1, %d0           | Add with x[i]
    move.l   #32767, %d1
    move.l   #65534, %a6
    add.l    %d1, %d0           | Bias result to [-1..65534]
    cmp.l    %a6, %d0           | Now do clip to [0..65534] range
    jls      2f
    jpl      1f
    clr.l    %d0                | Clip low
    .word    0x51fa             | trapf.w, shadow next insn
1:
    move.l   %a6, %d0           | Clip high
2:
    sub.l    %d1, %d0           | Bias clipped result back to [-32767..32767]
    move.w   %d0, (%a5)+        | Write result to y[i]
    neg.l    %d0                | msac.w is bugged in gas, do this for now
    move.l   (%a4)+, %a6        | Fetch den[0] and den[1]
    mac.w    %a6u, %d0l, %acc0
    mac.w    %a6l, %d0l, (%a4)+, %a6, %acc1
    mac.w    %a6u, %d0l, %acc2
    mac.w    %a6l, %d0l, (%a4)+, %a6, %acc3
    movclr.l %acc0, %d1
    add.l    %d2, %d1           | mem[0] = mem[1] - den[0]*y[i]
    movclr.l %acc1, %d2
    add.l    %d3, %d2           | mem[1] = mem[2] - den[1]*y[i]
    movclr.l %acc2, %d3
    add.l    %d4, %d3           | mem[2] = mem[3] - den[2]*y[i]
    movclr.l %acc3, %d4
    add.l    %d5, %d4           | mem[3] = mem[4] - den[3]*y[i]
    mac.w    %a6u, %d0l, %acc0
    mac.w    %a6l, %d0l, (%a4)+, %a6, %acc1
    mac.w    %a6u, %d0l, %acc2
    mac.w    %a6l, %d0l, (%a4)+, %a6, %acc3
    lea.l    (-20, %a4), %a4    | wrap den pointer back to den[0]
    movclr.l %acc0, %d5
    add.l    %d6, %d5           | mem[4] = mem[5] - den[4]*y[i]
    movclr.l %acc1, %d6
    add.l    %d7, %d6           | mem[5] = mem[6] - den[5]*y[i]
    movclr.l %acc2, %d7
    add.l    %a0, %d7           | mem[6] = mem[7] - den[6]*y[i]
    movclr.l %acc3, %a0
    add.l    %a1, %a0           | mem[7] = mem[8] - den[7]*y[i]
    mac.w    %a6u, %d0l, %acc0
    mac.w    %a6l, %d0l, %acc1
    movclr.l %acc0, %a1
    add.l    %a2, %a1           | mem[8] = mem[9] - den[8]*y[i]
    movclr.l %acc1, %a2         | mem[9] = -den[9]*y[i]

    subq.l   #1, (44+16, %sp)   | Have we done all samples?
    jne      0b
    move.l   (44+24, %sp), %a6  | Fetch mem pointer
    movem.l  %d1-%d7/%a0-%a2, (%a6) | Save back mem[]

.exit:
    movem.l  (%sp), %d2-%d7/%a2-%a6
    lea.l    (44, %sp), %sp
    rts


/* void qmf_synth(const spx_word16_t *x1, const spx_word16_t *x2, const spx_word16_t *a, spx_word16_t *y, int N, int M, spx_word32_t *mem1, spx_word32_t *mem2, char *stack) */
    .global qmf_synth
qmf_synth:
    lea.l    (-44, %sp), %sp
    movem.l  %d2-%d7/%a2-%a6, (%sp)
    movem.l  (44+4, %sp), %a0-%a3          | a0 = x1, a1 = x2, a2 = a, a3 = y
    movem.l  (44+20, %sp), %d0-%d1/%a4-%a5 | d0 = N, d1 = M, a4 = mem1,a5 = mem2
    move.l   #0x80, %macsr                 | Enable saturation

    | Comments make more sense when compared to the reference C version
    move.l   %a2, %d6                   | Backup a
    lsr.l    #1, %d0                    | N2 = N >> 1
    lsr.l    #1, %d1                    | M2 = M >> 1
    move.l   %d1, %d7                   | Backup M2
    clr.l    %d2
    sub.l    %d0, %d2
    sub.l    %d1, %d2                   | d2 = -(N2 + M2)
    lea.l    (%sp, %d2.l*2), %a2        | Alloc two buffers of N2 + M2 shorts
    lea.l    (%a2, %d2.l*2), %a6        | a2 = xx1, a6 = xx2
    move.l   %sp, %d3
    move.l   %a6, %sp                   | Update sp
    move.l   %d3, -(%sp)                | Stack old %sp

    | Backwards copy x1 and x2 arrays to xx1 and xx2, assume N2 is power of two
    | TODO: these copying loops probably have more potential for optimization
    lea.l    (%a0, %d0.l*2), %a0        | x1 += N2
    lea.l    (%a1, %d0.l*2), %a1        | x2 += N2
    move.l   %d0, %d2                   | Loop counter is N2
0:
    move.l   -(%a0), %d3
    swap.w   %d3
    move.l   %d3, (%a2)+
    move.l   -(%a1), %d3
    swap.w   %d3
    move.l   %d3, (%a6)+
    subq.l   #2, %d2
    jne      0b

    | Copy alternate members of mem1 and mem2 to last part of xx1 and xx2
    move.l   %d1, %d2                           | Loop counter is M2
    addq.l   #2, %a4                            | a4 = &mem1[1]
    addq.l   #2, %a5                            | a5 = &mem2[1]
    move.l   %a4, %d3                           | Backup mem1 and mem2
    move.l   %a5, %d4
0:
    move.w   (%a4), (%a2)+
    move.w   (%a5), (%a6)+ 
    addq.l   #4, %a4
    addq.l   #4, %a5
    subq.l   #1, %d2
    jne      0b
    move.l   %d3, %a4                           | a4 = &mem1[1]
    move.l   %d4, %a5                           | a5 = &mem2[1]

    clr.l    %d2
    sub.l    %d1, %d2                           | d2 = -M2
    lea.l    (-4, %a2, %d2.l*2), %a0            | a0 = &xx1[N2 - 2]
    lea.l    (-4, %a6, %d2.l*2), %a1            | a1 = &xx2[N2 - 2]
    move.l   %d6, %a2                           | a2 = a

    | Main loop, register usage:
    | d0 = N2 counter, d1 = M2 counter, d7 = M2 backup
    | d2 = x10, d3 = x11, d4 = x20, d5 = x21, d6 = [a0, a1]
    | a0 = xx1, a1 = xx2, a2 = a, a3 = y, a4 = mem1, a5 = mem2
0:  | Outerloop
    move.l   #32768, %d2                        | Rounding constant
    move.l   %d2, %acc0
    move.l   %d2, %acc1
    move.l   %d2, %acc2
    move.l   %d2, %acc3
    move.w   (%a0)+, %d2                        | d2 = x10
    move.w   (%a1)+, %d4                        | d4 = x20
    move.l   (%a2)+, %d6                        | d6 = [a0, a1]
1:  | Innerloop
    move.w   (%a0)+, %d3                        | d3 = x11
    move.w   (%a1)+, %d5                        | d5 = x21
    mac.w    %d6u, %d3l, <<, %acc0              | acc0 += a0*x11
    msac.w   %d6u, %d5l, <<, %acc0              | acc0 -= a0*x21
    mac.w    %d6l, %d3l, <<, %acc1              | acc1 += a1*x11
    mac.w    %d6l, %d5l, <<, %acc1              | acc1 += a1*x21
    mac.w    %d6u, %d2l, <<, %acc2              | acc2 += a0*x10
    msac.w   %d6u, %d4l, <<, %acc2              | acc2 -= a0*x20
    mac.w    %d6l, %d2l, <<, %acc3              | acc3 += a1*x10
    mac.w    %d6l, %d4l, <<, (%a2)+, %d6, %acc3 | acc3 += a1*x20

    move.w   (%a0)+, %d2                        | d2 = x10
    move.w   (%a1)+, %d4                        | d4 = x20
    mac.w    %d6u, %d2l, <<, %acc0              | acc0 += a0*x10
    msac.w   %d6u, %d4l, <<, %acc0              | acc0 -= a0*x20
    mac.w    %d6l, %d2l, <<, %acc1              | acc1 += a1*x10
    mac.w    %d6l, %d4l, <<, %acc1              | acc1 += a1*x20
    mac.w    %d6u, %d3l, <<, %acc2              | acc2 += a0*x11
    msac.w   %d6u, %d5l, <<, %acc2              | acc2 -= a0*x21
    mac.w    %d6l, %d3l, <<, %acc3              | acc3 += a1*x11
    mac.w    %d6l, %d5l, <<, (%a2)+, %d6, %acc3 | acc3 += a1*x21
    subq.l   #2, %d1
    jne      1b
 
    sub.l    %d7, %d1                           | d1 = -M2
    lea.l    (-4, %a2, %d1.l*4), %a2            | a2 = &a[0]
    lea.l    (-6, %a0, %d1.l*2), %a0            | a0 = &xx1[N2 - 2 - i] 
    lea.l    (-6, %a1, %d1.l*2), %a1            | a1 = &xx2[N2 - 2 - i]
    neg.l    %d1                                | d1 = M2
    movclr.l %acc0, %d2
    movclr.l %acc1, %d3
    movclr.l %acc2, %d4
    movclr.l %acc3, %d5
    swap.w   %d2                                | Shift 16 right
    swap.w   %d3
    swap.w   %d4
    swap.w   %d5
    | Thanks to the extra shift in the mac chain, we get clipping for free.
    | The clipping will be [-32768..32767], not Speex standard [-32767..32767],
    | but since qmf_synth() is called so late in the signal chain, it should
    | work fine.
    move.w   %d2, (%a3)+                        | Write results to y[]
    move.w   %d3, (%a3)+
    move.w   %d4, (%a3)+
    move.w   %d5, (%a3)+
    subq.l   #2, %d0
    jne      0b

    | Copy start of xx1 and xx2 back to alternate mem1 and mem2 entries
    addq.l   #4, %a0                            | a0 = &xx1[0]
    addq.l   #4, %a1                            | a1 = &xx2[0]
0:
    move.w   (%a0)+, (%a4)
    move.w   (%a1)+, (%a5)
    addq.l   #4, %a4
    addq.l   #4, %a5
    subq.l   #1, %d1
    jne      0b

    move.l   #0, %macsr
    move.l   (%sp), %sp
    movem.l  (%sp), %d2-%d7/%a2-%a6
    lea.l    (44, %sp), %sp
    rts


/* void signal_mul(const spx_sig_t *x, spx_sig_t *y, spx_word32_t scale, int len) */
    .global signal_mul
signal_mul:
    lea.l    (-20, %sp), %sp
    movem.l  %d2-%d6, (%sp)
    movem.l  (20+4, %sp), %a0-%a1           | a0 = x, a1 = y
    movem.l  (20+12, %sp), %d0-%d1          | d0 = scale, d1 = len
    moveq.l  #0x20, %d6
    move.l   %d6, %macsr                    | Set MAC unit to fractional mode
    asl.l    #3, %d0                        | Pre-scale 'scale'
    moveq.l  #9, %d6
0:
    movem.l  (%a0), %d2-%d5                 | Fetch input
    asl.l    %d6, %d2                       | Shift each value 9 to the left
    asl.l    %d6, %d3
    asl.l    %d6, %d4
    asl.l    %d6, %d5
    mac.l    %d2, %d0, %acc0                | Do multiplies
    mac.l    %d3, %d0, %acc1
    mac.l    %d4, %d0, %acc2
    mac.l    %d5, %d0, %acc3
    lea.l    (16, %a0), %a0
    movclr.l %acc0, %d2
    movclr.l %acc1, %d3
    movclr.l %acc2, %d4
    movclr.l %acc3, %d5
    asl.l    #5, %d2                        | Adjust to proper format
    asl.l    #5, %d3
    asl.l    #5, %d4
    asl.l    #5, %d5
    movem.l  %d2-%d5, (%a1)                 | Save output
    lea.l    (16, %a1), %a1
    subq.l   #4, %d1
    jne      0b

    clr.l    %d0
    move.l   %d0, %macsr                    | Set MAC unit back to integer mode
    movem.l  (%sp), %d2-%d6
    lea.l    (20, %sp), %sp
    rts

