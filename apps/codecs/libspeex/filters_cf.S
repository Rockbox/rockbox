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
    move.l   #32768, %d1
    add.l    %d1, %d0           | Bias result to [0..65535]
    cmp.l    #65535, %d0        | Clip to [0..65535] range
    jle      1f
    spl.b    %d0                
    ext.w    %d0
1:
    sub.l    %d1, %d0           | Bias clipped result back to [-32768..32767]
    neg.l    %d0                | msac.w is bugged in gas, do this for now
    move.w   %d0, (%a5)+        | Write result to y[i]
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
    move.l   #32768, %d1
    add.l    %d1, %d0           | Bias result to [0..65535]
    cmp.l    #65535, %d0        | Clip to [0..65535] range
    jle      1f
    spl.b    %d0                
    ext.w    %d0
1:
    sub.l    %d1, %d0           | Bias clipped result back to [-32768..32767]
    neg.l    %d0                | msac.w is bugged in gas, do this for now
    move.w   %d0, (%a5)+        | Write result to y[i]
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
    movem.l (%sp), %d2-%d7/%a2-%a6
    lea.l (44, %sp), %sp
    rts

