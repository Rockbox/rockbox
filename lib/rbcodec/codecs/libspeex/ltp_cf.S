/* Copyright (C) 2007 Thom Johansen */
/**
   @file ltp_cf.S
   @brief Long-Term Prediction functions (Coldfire version)
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
/* spx_word32_t inner_prod(const spx_word16_t *x, const spx_word16_t *y, int len) */
    .global inner_prod
inner_prod:
    lea.l    (-28, %sp), %sp
    movem.l  %d2-%d7/%a2, (%sp)
    movem.l  (28+4, %sp), %a0-%a1 | a0 = x, a1 = y
    move.l   (28+12, %sp), %d0    | d0 = len

    | We assume we're never called with a 'len' of zero
    btst     #2, %d0            | Check if we need to do one round of four
    jeq      0f                 | samples before we do runs of eight
    movem.l  (%a0), %d1-%d2     | Fetch four samples from x
    movem.l  (%a1), %d3-%d4     | Fetch four samples from y
    mac.w    %d1u, %d3u, %acc0
    mac.w    %d1l, %d3l, %acc0
    mac.w    %d2u, %d4u, %acc0
    mac.w    %d2l, %d4l, %acc0
    addq.l   #8, %a0
    addq.l   #8, %a1
    subq.l   #4, %d0
    jeq      .save

0:
    movem.l  (%a0), %d1-%d4     | Fetch eight samples from x
    movem.l  (%a1), %d5-%d7/%a2 | Fetch eight samples from y
    mac.w    %d1u, %d5u, %acc0
    mac.w    %d1l, %d5l, %acc0
    mac.w    %d2u, %d6u, %acc0
    mac.w    %d2l, %d6l, %acc0
    mac.w    %d3u, %d7u, %acc0
    mac.w    %d3l, %d7l, %acc0
    mac.w    %d4u, %a2u, %acc0
    mac.w    %d4l, %a2l, %acc0
    lea.l    (16, %a0), %a0
    lea.l    (16, %a1), %a1
    subq.l   #8, %d0
    jne      0b

.save:
    move.l   %accext01, %d1     | Fetch top 8 bits of answer
    movclr.l %acc0, %d0         | Fetch lower 32 bits
    lsr.l    #6, %d0
    moveq.l  #26, %d2
    asl.l    %d2, %d1
    or.l     %d1, %d0           | Combine (top << 26) | (lower >> 6)

    movem.l  (%sp), %d2-%d7/%a2
    lea.l    (28, %sp), %sp
    rts

