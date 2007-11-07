/* Copyright (C) 2007 Thom Johansen */
/**
   @file filters_arm4.S
   @brief Various analysis/synthesis filters (ARMv4 version)
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
    stmdb   sp!, { r4-r11, lr }
    ldr     r5, [sp, #36]           @ r0 = x, r1 = den, r2 = y, r3 = N
    ldr     r4, [sp, #40]           @ r4 = mem, r5 = ord
    cmp     r5, #10
    beq     .order_10
    cmp     r5, #8
    beq     .order_8
    ldmia   sp!, { r4-r11, pc }     @ Mon-supported order, return

    @ TODO: try using direct form 1 filtering
.order_8:
    ldmia   r4, { r5-r12 }          @ r5-r12 = mem[0..7]
0:
    add     r5, r5, #4096           @ Rounding constant
    ldrsh   r14, [r0], #2
    add     r14, r14, r5, asr #13   @ (mem[0] + 4096) >> 13 + x[i]
    mov     r5, #0x7f00
    orr     r5, r5, #0xff           @ r5 = 32767
    cmp     r14, r5
    movgt   r14, r5                 @ Clip positive
    cmn     r14, r5
    rsblt   r14, r5, #0             @ Clip negative
    strh    r14, [r2], #2           @ Write result to y[i]

    ldrsh   r4, [r1]
    mul     r5, r4, r14 
    sub     r5, r6, r5              @ mem[0] = mem[1] - den[0]*y[i]
    ldrsh   r4, [r1, #2]
    mul     r6, r4, r14 
    sub     r6, r7, r6              @ mem[1] = mem[2] - den[1]*y[i]
    ldrsh   r4, [r1, #4]
    mul     r7, r4, r14 
    sub     r7, r8, r7              @ mem[2] = mem[3] - den[2]*y[i]
    ldrsh   r4, [r1, #6]
    mul     r8, r4, r14 
    sub     r8, r9, r8              @ mem[3] = mem[4] - den[3]*y[i]
    ldrsh   r4, [r1, #8]
    mul     r9, r4, r14 
    sub     r9, r10, r9             @ mem[4] = mem[5] - den[4]*y[i]
    ldrsh   r4, [r1, #10]
    mul     r10, r4, r14 
    sub     r10, r11, r10           @ mem[5] = mem[6] - den[5]*y[i]
    ldrsh   r4, [r1, #12]
    mul     r11, r4, r14 
    sub     r11, r12, r11           @ mem[6] = mem[7] - den[6]*y[i]
    ldrsh   r4, [r1, #14]
    mul     r12, r4, r14 
    rsb     r12, r12, #0            @ mem[7] = -den[7]*y[i]
    subs    r3, r3, #1
    bne     0b
    ldr     r4, [sp, #40]           @ r4 = mem
    stmia   r4, { r5-r12 }          @ Save back mem[]
    ldmia   sp!, { r4-r11, pc }     @ Exit

.order_10:
    ldmia   r4, { r5-r9 }           @ r5-r9 = mem[0..4]
    add     r5, r5, #4096           @ Rounding constant
    ldrsh   r14, [r0], #2
    add     r14, r14, r5, asr #13   @ (mem[0] + 4096) >> 13 + x[i]
    mov     r5, #0x7f00
    orr     r5, r5, #0xff           @ r5 = 32767
    cmp     r14, r5
    movgt   r14, r5                 @ Clip positive
    cmn     r14, r5
    rsblt   r14, r5, #0             @ Clip negative
    strh    r14, [r2], #2           @ Write result to y[i]

    ldmia   r1!, { r10-r12 }        @ r10-r12 = den[0..5]
    mov     r5, r10, lsl #16
    mov     r5, r5, asr #16
    mul     r5, r14, r5 
    sub     r5, r6, r5              @ mem[0] = mem[1] - den[0]*y[i]
    mov     r10, r10, asr #16
    mul     r6, r14, r10 
    sub     r6, r7, r6              @ mem[1] = mem[2] - den[1]*y[i]
    mov     r10, r11, lsl #16
    mov     r10, r10, asr #16
    mul     r7, r14, r10 
    sub     r7, r8, r7              @ mem[2] = mem[3] - den[2]*y[i]
    mov     r10, r11, asr #16
    mul     r8, r14, r10 
    sub     r8, r9, r8              @ mem[3] = mem[4] - den[3]*y[i]
    stmia   r4!, { r5-r8 }          @ Write back mem[0..3], r4 = &mem[4]
    mov     r10, r12, lsl #16
    mov     r10, r10, asr #16
    mul     r5, r14, r10 

    ldmib   r4, { r6-r10 }          @ r6-r10 = mem[5..9]
    sub     r5, r6, r5              @ mem[4] = mem[5] - den[4]*y[i]
    mov     r12, r12, asr #16
    mul     r6, r14, r12 
    sub     r6, r7, r6              @ mem[5] = mem[6] - den[5]*y[i]
    ldmia   r1!, { r11-r12 }        @ r11-r12 = den[6..9]
    mov     r7, r11, lsl #16
    mov     r7, r7, asr #16
    mul     r7, r14, r7 
    sub     r7, r8, r7              @ mem[6] = mem[7] - den[6]*y[i]
    mov     r11, r11, asr #16
    mul     r8, r14, r11 
    sub     r8, r9, r8              @ mem[7] = mem[8] - den[7]*y[i]
    mov     r11, r12, lsl #16
    mov     r11, r11, asr #16
    mul     r9, r14, r11 
    sub     r9, r10, r9             @ mem[8] = mem[9] - den[8]*y[i]
    mov     r12, r12, asr #16
    mul     r10, r14, r12 
    rsb     r10, r10, #0            @ mem[9] = -den[9]*y[i]
    stmia   r4!, { r5-r10 }         @ Write back mem[4..9]
    sub     r4, r4, #10*4
    sub     r1, r1, #10*2
    subs    r3, r3, #1
    bne     .order_10 
    ldmia   sp!, { r4-r11, pc }     @ Exit


/* void qmf_synth(const spx_word16_t *x1, const spx_word16_t *x2, const spx_word16_t *a, spx_word16_t *y, int N, int M, spx_word32_t *mem1, spx_word32_t *mem2, char *stack) */
    .global qmf_synth
qmf_synth:
    stmdb   sp!, { r4-r11, lr }
    add     r7, sp, #36             @ r0 = x1, r1 = x2, r2 = a, r3 = y
    ldmia   r7, { r4-r7 }           @ r4 = N, r5 = M, r6 = mem1, r7 = mem2

    add     r8, r4, r5
    sub     r9, sp, r8              @ r9 = sp - (N + M >> 1) = xx2
    sub     r8, r9, r8              @ r8 = r9 - (N + M >> 1) = xx1
    str     sp, [r8, #-4]           @ Stack old sp
    sub     sp, r8, #4              @ Update sp

    add     r0, r0, r4              @ x1 += N >> 1
    add     r1, r1, r4              @ x2 += N >> 1
    mov     r14, r4                 @ Loop counter is N
0:
    @ Backwards copy x1 and x2 arrays to xx1 and xx2, assume N2 is power of two
    @ N should always be a multiple of four, so this should be OK
    ldmdb   r0!, { r10-r11 }
    mov     r12, r10, ror #16
    mov     r11, r11, ror #16
    stmia   r8!, { r11-r12 }
    ldmdb   r1!, { r10-r11 }
    mov     r12, r10, ror #16
    mov     r11, r11, ror #16
    stmia   r9!, { r11-r12 }
    subs    r14, r14, #8
    bne     0b

    @ Copy alternate members of mem1 and mem2 to last part of xx1 and xx2
    mov     r14, r5                 @ Loop counter is M
    add     r6, r6, #2
    add     r7, r7, #2
    stmdb   sp!, { r6-r7 }          @ Stack &mem1[1], &mem2[1]
0:
    ldrh    r10, [r6], #4
    ldrh    r11, [r6], #4
    ldrh    r12, [r7], #4
    orr     r10, r10, r11, lsl #16
    ldrh    r11, [r7], #4
    orr     r11, r12, r11, lsl #16
    str     r10, [r8], #4
    str     r11, [r9], #4
    subs    r14, r14, #4
    bne     0b

    sub     r0, r8, r5              @ r0 = &xx1[N2]
    sub     r1, r9, r5              @ r1 = &xx2[N2]
    str     r4, [sp, #-4]           @ Stack N
    mov     r4, r5
    str     r4, [sp, #-8]           @ Stack M
    @ sp doesn't point to the end of the stack frame from here on, but we're not
    @ calling anything so it shouldn't matter
    @ Main loop, register usage:
    @ r0 = xx1, r1 = xx2, r2 = a, r3 = y, r4 = M, r5 = x10, r6 = x11, r7 = x20
    @ r8 = x21, r9 = [a1, a0], r10 = acc0, r11 = acc1, r12 = acc2, r14 = acc3
0:  @ Outerloop
    mov     r10, #16384             @ Init acccumulators to rounding const
    mov     r11, #16384
    mov     r12, #16384
    mov     r14, #16384

    ldrsh   r5, [r0, #-4]!          @ r5 = x10, r0 = &xx1[N2 - 2]
    ldrsh   r7, [r1, #-4]!          @ r7 = x20, r1 = &xx2[N2 - 2]
1:  @ Innerloop
    ldrsh   r9, [r2], #2            @ r9 = a0
    ldrsh   r6, [r0, #2]!           @ r6 = x11
    ldrsh   r8, [r1, #2]!           @ r8 = x21
    sub     r5, r5, r7              @ r5 = x10 - x20
    add     r7, r5, r7, asl #1      @ r7 = x10 + x20
    mla     r12, r9, r5, r12        @ acc2 += a0*(x10 - x20)
    sub     r5, r6, r8              @ r5 = x11 - x21
    mla     r10, r9, r5, r10        @ acc0 += a0*(x11 - x21)
    ldrsh   r9, [r2], #2            @ r9 = a1
    add     r5, r6, r8              @ r5 = x11 + x21
    mla     r14, r9, r7, r14        @ acc3 += a1*(x10 + x20)
    mla     r11, r9, r5, r11        @ acc1 += a1*(x11 + x21)

    ldrsh   r9, [r2], #2            @ r9 = a1
    ldrsh   r5, [r0, #2]!           @ r5 = x10
    ldrsh   r7, [r1, #2]!           @ r7 = x20
    sub     r6, r6, r8              @ r6 = x11 - x21
    add     r8, r6, r8, asl #1      @ r8 = x11 + x21
    mla     r12, r9, r6, r12        @ acc2 += a0*(x11 - x21)
    sub     r6, r5, r7              @ r6 = x10 - x20
    mla     r10, r9, r6, r10        @ acc0 += a0*(x10 - x20)
    ldrsh   r9, [r2], #2            @ r9 = a1
    add     r6, r5, r7              @ r5 = x10 + x20
    mla     r14, r9, r8, r14        @ acc3 += a1*(x11 + x21)
    mla     r11, r9, r6, r11        @ acc1 += a1*(x10 + x10)
    subs    r4, r4, #4
    bne     1b

    ldr     r4, [sp, #-8]           @ r4 = M
    sub     r2, r2, r4, lsl #1      @ r2 = &a[0]
    sub     r0, r0, r4              @ r0 = &xx1[N2 - 2 - i]
    sub     r1, r1, r4              @ r1 = &xx2[N2 - 2 - i]
    
    mov     r10, r10, asr #15       @ Shift outputs down
    mov     r11, r11, asr #15
    mov     r12, r12, asr #15
    mov     r14, r14, asr #15

    @ TODO: this can be optimized further
    mov     r9, #0x7f00             @ Clip all four outputs
    orr     r9, r9, #0xff           @ r9 = 32767
    cmp     r10, r9
    movgt   r10, r9
    cmn     r10, r9
    rsblt   r10, r9, #0 
    cmp     r11, r9
    movgt   r11, r9
    cmn     r11, r9
    rsblt   r11, r9, #0 
    cmp     r12, r9
    movgt   r12, r9
    cmn     r12, r9
    rsblt   r12, r9, #0 
    cmp     r14, r9
    movgt   r14, r9
    cmn     r14, r9
    rsblt   r14, r9, #0 

    strh    r10, [r3], #2           @ Write outputs
    strh    r11, [r3], #2
    strh    r12, [r3], #2
    strh    r14, [r3], #2
    ldr     r10, [sp, #-4]          @ Load N
    subs    r10, r10, #4            @ Are we done?
    strne   r10, [sp, #-4]
    bne     0b

    @ Copy start of xx1 and xx2 back to alternate mem1 and mem2 entries
    @ r0 and r1 are &xx1[0] and &xx2[0] at this point
    ldmia   sp, { r5-r6, sp }       @ Fetch &mem1[1], &mem2[1], restore sp
0:
    ldr     r7, [r0], #4
    ldr     r8, [r1], #4
    strh    r7, [r5], #4
    strh    r8, [r6], #4
    mov     r7, r7, lsr #16
    mov     r8, r8, lsr #16
    strh    r7, [r5], #4
    strh    r8, [r6], #4
    subs    r4, r4, #4
    bne     0b
    ldmia   sp!, { r4-r11, pc }     @ Exit


/* void signal_mul(const spx_sig_t *x, spx_sig_t *y, spx_word32_t scale, int len) */
    .global signal_mul
signal_mul:
    stmdb   sp!, { r4-r8, lr }
0:
    ldmia   r0!, { r5-r8 }                  @ Load four input samples
    smull   r5, r12, r2, r5
    mov     r12, r12, lsl #18               @ Recombine upper and lower parts
    orr     r5, r12, r5, lsr #14
    smull   r6, r12, r2, r6
    mov     r12, r12, lsl #18
    orr     r6, r12, r6, lsr #14
    smull   r7, r12, r2, r7
    mov     r12, r12, lsl #18
    orr     r7, r12, r7, lsr #14
    smull   r8, r12, r2, r8
    mov     r12, r12, lsl #18
    orr     r8, r12, r8, lsr #14
    stmia   r1!, { r5-r8 }                  @ Store four output samples
    subs    r3, r3, #4                      @ Are we done?
    bne     0b

    ldmia   sp!, { r4-r8, pc }              @ Exit

