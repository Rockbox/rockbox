/*

libdemac - A Monkey's Audio decoder

$Id$

Copyright (C) Dave Chapman 2007

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA

*/
#include "demac_config.h"

    .section    ICODE_SECTION_DEMAC_ARM,"ax",%progbits

    .align      2

    .global     predictor_decode_stereo
    .type       predictor_decode_stereo,%function


/* NOTE: The following need to be kept in sync with parser.h */

#define YDELAYA        200
#define YDELAYB        168
#define XDELAYA        136
#define XDELAYB        104
#define YADAPTCOEFFSA   72
#define XADAPTCOEFFSA   56
#define YADAPTCOEFFSB   40
#define XADAPTCOEFFSB   20

/* struct predictor_t members: */
#define buf              0    /* int32_t* buf */

#define YlastA           4    /* int32_t YlastA; */
#define XlastA           8    /* int32_t XlastA; */

#define YfilterB        12    /* int32_t YfilterB; */
#define XfilterA        16    /* int32_t XfilterA; */

#define XfilterB        20    /* int32_t XfilterB; */
#define YfilterA        24    /* int32_t YfilterA; */
	
#define YcoeffsA        28    /* int32_t YcoeffsA[4]; */
#define XcoeffsA        44    /* int32_t XcoeffsA[4]; */
#define YcoeffsB        60    /* int32_t YcoeffsB[5]; */
#define XcoeffsB        80    /* int32_t XcoeffsB[5]; */

#define historybuffer  100    /* int32_t historybuffer[] */

@ Register usage:
@
@ r0-r11 - scratch
@ r12 - struct predictor_t* p
@ r14 - int32_t* p->buf

@ void predictor_decode_stereo(struct predictor_t* p,
@                              int32_t* decoded0,
@                              int32_t* decoded1,
@                              int count)

predictor_decode_stereo:
    stmdb   sp!, {r1-r11, lr}

    @ r1 (decoded0) is [sp]
    @ r2 (decoded1) is [sp, #4]
    @ r3 (count)    is [sp, #8]

    mov     r12, r0       @ r12 := p
    ldr     r14, [r0]     @ r14 := p->buf

loop:

@@@@@@@@@@@@@@@@@@@@@@@@@@@ PREDICTOR Y

@ Predictor Y, Filter A

    ldr     r10, [r12, #YlastA]     @ r10 := p->YlastA
    add     r11, r14, #YDELAYA-12   @ r11 := &p->buf[YDELAYA-3]

    ldmia   r11, { r2 - r4 }        @ r2 := p->buf[YDELAYA-3]
                                    @ r3 := p->buf[YDELAYA-2]
                                    @ r4 := p->buf[YDELAYA-1]

    subs    r4, r10, r4             @ r4 := r10 - r4

    add     r1, r12, #YcoeffsA
    ldmia   r1, {r6 - r9}           @ r6 := p->YcoeffsA[0]
                                    @ r7 := p->YcoeffsA[1]
                                    @ r8 := p->YcoeffsA[2]
                                    @ r9 := p->YcoeffsA[3]

    mul     r0, r10, r6             @ r0 := p->buf[YDELAYA] * p->YcoeffsA[0]
    mla     r0, r4, r7, r0          @ r0 += p->buf[YDELAYA-1] * p->YcoeffsA[1]
    mla     r0, r3, r8, r0          @ r0 += p->buf[YDELAYA-2] * p->YcoeffsA[2]
    mla     r0, r2, r9, r0          @ r0 += p->buf[YDELAYA-3] * p->YcoeffsA[3]

    add     r11, r14, #YDELAYA-4
    stmia   r11, { r4, r10 }        @ p->buf[YDELAYA-1] = r4
                                    @ p->buf[YDELAYA] = r10

    @ flags were set above, in the subs instruction
    mvngt   r4, #0
    movlt   r4, #1                  @ r4 := SIGN(r4) (see .c for SIGN macro)

    cmp     r10, #0
    mvngt   r10, #0
    movlt   r10, #1                 @ r10 := SIGN(r10) (see .c for SIGN macro)

    add     r1, r14, #YADAPTCOEFFSA-4
    stmia   r1, {r4, r10}           @ p->buf[YADAPTCOEFFSA-1] := r4
                                    @ p->buf[YADAPTCOEFFSA] := r10

    @ NOTE: r0 now contains predictionA - don't overwrite.

@ Predictor Y, Filter B

    add     r2, r12, #YfilterB
    ldmia   r2, {r2, r11}           @ r2 := p->YfilterB
                                    @ r11 := p->XfilterA

    rsb     r2, r2, r2, lsl #5      @ r2 := r2 * 32 - r2 ( == r2*31)
    sub     r10, r11, r2, asr #5    @ r10 (p->buf[YDELAYB]) := r11 - (r2 >> 5)

    str     r11, [r12, #YfilterB]   @ p->YfilterB := r11 (p->XfilterA)

    add     r11, r14, #YDELAYB-16   @ r11 := &p->buf[YDELAYB-4]

    ldmia   r11, { r2 - r5 }        @ r2 := p->buf[YDELAYB-4]
                                    @ r3 := p->buf[YDELAYB-3]
                                    @ r4 := p->buf[YDELAYB-2]
                                    @ r5 := p->buf[YDELAYB-1]

    subs    r5, r10, r5             @ r5 := r10 - r5

    add     r1, r12, #YcoeffsB
    ldmia   r1, {r6,r7,r8,r9,r11}   @ r6 := p->YcoeffsB[0]
                                    @ r7 := p->YcoeffsB[1]
                                    @ r8 := p->YcoeffsB[2]
                                    @ r9 := p->YcoeffsB[3]
                                    @ r11 := p->YcoeffsB[4]

    mul     r1, r10, r6             @ r1 := p->buf[YDELAYB] * p->YcoeffsB[0]
    mla     r1, r5, r7, r1          @ r1 += p->buf[YDELAYB-1] * p->YcoeffsB[1]
    mla     r1, r4, r8, r1          @ r1 += p->buf[YDELAYB-2] * p->YcoeffsB[2]
    mla     r1, r3, r9, r1          @ r1 += p->buf[YDELAYB-3] * p->YcoeffsB[3]
    mla     r1, r2, r11, r1         @ r1 += p->buf[YDELAYB-4] * p->YcoeffsB[4]

    add     r2, r14, #YDELAYB-4     @ r2 := &p->buf[YDELAYB-1]
    stmia   r2, { r5, r10 }         @ p->buf[YDELAYB-1] = r5
                                    @ p->buf[YDELAYB] = r10

    @ flags were set above, in the subs instruction
    mvngt   r5, #0
    movlt   r5, #1                 @ r5 := SIGN(r5) (see .c for SIGN macro)

    cmp     r10, #0
    mvngt   r10, #0
    movlt   r10, #1                @ r10 := SIGN(r10) (see .c for SIGN macro)

    add     r2, r14, #YADAPTCOEFFSB-4
    stmia   r2, {r5, r10}           @ p->buf[YADAPTCOEFFSB-1] := r5
                                    @ p->buf[YADAPTCOEFFSB] := r10

    @ r0 still contains predictionA
    @ r1 contains predictionB

    @ Finish Predictor Y

    ldr     r2, [sp]                @ r2 := decoded0
    add     r0, r0, r1, asr #1      @ r0 := r0 + (r1 >> 1)
    ldr     r3, [r2]                @ r3 := *decoded0
    add     r1, r3, r0, asr #10     @ r1 := r3 + (r0 >> 10)
    str     r1, [r12, #YlastA]      @ p->YlastA := r1

    ldr     r4, [r12, #YfilterA]    @ r4 := p->YfilterA
    rsb     r4, r4, r4, lsl #5      @ r4 := r4 * 32 - r4 ( == r4*31)
    add     r1, r1, r4, asr #5      @ r1 := r1 + (r4 >> 5)
    str     r1, [r12, #YfilterA]    @ p->YfilterA := r1

    @ r1 contains p->YfilterA
    @ r2 contains decoded0
    @ r3 contains *decoded0

    @ r6, r7, r8, r9, r11 contain p->YcoeffsB[0..4]
    @ r5, r10 contain p->buf[YADAPTCOEFFSB-1] and p->buf[YADAPTCOEFFSB]

    cmp     r3, #0
    stmia   r2!, {r1}               @ *(decoded0++) := r1  (p->YfilterA)
    str     r2, [sp]                @ save decoded0
    beq     2f

    add     r1, r14, #YADAPTCOEFFSB-16
    ldmia   r1, { r2, r3, r4 }      @ r2 := p->buf[YADAPTCOEFFSB-4]
                                    @ r3 := p->buf[YADAPTCOEFFSB-3]
                                    @ r4 := p->buf[YADAPTCOEFFSB-2]
    blt     1f

    @ *decoded0 > 0

    sub     r6, r6, r10       @ r6 := p->YcoeffsB[0] - p->buf[YADAPTCOEFFSB]
    sub     r7, r7, r5        @ r7 := p->YcoeffsB[1] - p->buf[YADAPTCOEFFSB-1]
    sub     r8, r8, r4        @ r8 := p->YcoeffsB[2] - p->buf[YADAPTCOEFFSB-2]
    sub     r9, r9, r3        @ r9 := p->YcoeffsB[3] - p->buf[YADAPTCOEFFSB-3]
    sub     r11, r11, r2      @ r11 := p->YcoeffsB[4] - p->buf[YADAPTCOEFFSB-4]

    add     r0, r12, #YcoeffsB      
    stmia   r0, {r6,r7,r8,r9,r11}   @ Save p->YcoeffsB[]

    add     r1, r12, #YcoeffsA
    ldmia   r1, { r2-r5 }           @ r2 := p->YcoeffsA[0]
                                    @ r3 := p->YcoeffsA[1]
                                    @ r4 := p->YcoeffsA[2]
                                    @ r5 := p->YcoeffsA[3]

    add     r0, r14, #YADAPTCOEFFSA-12
    ldmia   r0, { r6-r9}            @ r6 := p->buf[YADAPTCOEFFSA-3]
                                    @ r7 := p->buf[YADAPTCOEFFSA-2]
                                    @ r8 := p->buf[YADAPTCOEFFSA-1]
                                    @ r9 := p->buf[YADAPTCOEFFSA]

    sub     r2, r2, r9        @ r2 := p->YcoeffsA[0] - p->buf[YADAPTCOEFFSA]
    sub     r3, r3, r8        @ r3 := p->YcoeffsA[1] - p->buf[YADAPTCOEFFSA-1]
    sub     r4, r4, r7        @ r4 := p->YcoeffsA[2] - p->buf[YADAPTCOEFFSA-2]
    sub     r5, r5, r6        @ r5 := p->YcoeffsA[3] - p->buf[YADAPTCOEFFSA-3]
    
    stmia   r1, {r2-r5}             @ Save p->YcoeffsA
    b       2f


1:  @ *decoded0 < 0

    add     r6, r6, r10       @ r6 := p->YcoeffsB[0] + p->buf[YADAPTCOEFFSB]
    add     r7, r7, r5        @ r7 := p->YcoeffsB[1] + p->buf[YADAPTCOEFFSB-1]
    add     r8, r8, r4        @ r8 := p->YcoeffsB[2] + p->buf[YADAPTCOEFFSB-2]
    add     r9, r9, r3        @ r9 := p->YcoeffsB[3] + p->buf[YADAPTCOEFFSB-3]
    add     r11, r11, r2      @ r11 := p->YcoeffsB[4] + p->buf[YADAPTCOEFFSB-4]

    add     r0, r12, #YcoeffsB      
    stmia   r0, {r6,r7,r8,r9,r11}   @ Save p->YcoeffsB[]

    add     r1, r12, #YcoeffsA
    ldmia   r1, { r2-r5 }           @ r2 := p->YcoeffsA[0]
                                    @ r3 := p->YcoeffsA[1]
                                    @ r4 := p->YcoeffsA[2]
                                    @ r5 := p->YcoeffsA[3]

    add     r0, r14, #YADAPTCOEFFSA-12
    ldmia   r0, { r6-r9}            @ r6 := p->buf[YADAPTCOEFFSA-3]
                                    @ r7 := p->buf[YADAPTCOEFFSA-2]
                                    @ r8 := p->buf[YADAPTCOEFFSA-1]
                                    @ r9 := p->buf[YADAPTCOEFFSA]

    add     r2, r2, r9        @ r2 := p->YcoeffsA[0] + p->buf[YADAPTCOEFFSA]
    add     r3, r3, r8        @ r3 := p->YcoeffsA[1] + p->buf[YADAPTCOEFFSA-1]
    add     r4, r4, r7        @ r4 := p->YcoeffsA[2] + p->buf[YADAPTCOEFFSA-2]
    add     r5, r5, r6        @ r5 := p->YcoeffsA[3] + p->buf[YADAPTCOEFFSA-3]
    
    stmia   r1, {r2-r5}       @ Save p->YcoeffsA

2:

@@@@@@@@@@@@@@@@@@@@@@@@@@@ PREDICTOR X

@ Predictor X, Filter A

    ldr     r10, [r12, #XlastA]     @ r10 := p->XlastA
    add     r11, r14, #XDELAYA-12   @ r11 := &p->buf[XDELAYA-3]

    ldmia   r11, { r2 - r4 }        @ r2 := p->buf[XDELAYA-3]
                                    @ r3 := p->buf[XDELAYA-2]
                                    @ r4 := p->buf[XDELAYA-1]

    subs    r4, r10, r4             @ r4 := r10 - r4

    add     r1, r12, #XcoeffsA
    ldmia   r1, {r6 - r9}           @ r6 := p->XcoeffsA[0]
                                    @ r7 := p->XcoeffsA[1]
                                    @ r8 := p->XcoeffsA[2]
                                    @ r9 := p->XcoeffsA[3]

    mul     r0, r10, r6             @ r0 := p->buf[XDELAYA] * p->XcoeffsA[0]
    mla     r0, r4, r7, r0          @ r0 += p->buf[XDELAYA-1] * p->XcoeffsA[1]
    mla     r0, r3, r8, r0          @ r0 += p->buf[XDELAYA-2] * p->XcoeffsA[2]
    mla     r0, r2, r9, r0          @ r0 += p->buf[XDELAYA-3] * p->XcoeffsA[3]

    add     r11, r14, #XDELAYA-4
    stmia   r11, { r4, r10 }        @ p->buf[XDELAYA-1] = r4
                                    @ p->buf[XDELAYA] = r10

    @ flags were set above, in the subs instruction
    mvngt   r4, #0
    movlt   r4, #1                 @ r4 := SIGN(r4) (see .c for SIGN macro)

    cmp     r10, #0
    mvngt   r10, #0
    movlt   r10, #1                @ r10 := SIGN(r10) (see .c for SIGN macro)

    add     r1, r14, #XADAPTCOEFFSA-4
    stmia   r1, {r4, r10}           @ p->buf[XADAPTCOEFFSA-1] := r4
                                    @ p->buf[XADAPTCOEFFSA] := r10

    @ NOTE: r0 now contains predictionA - don't overwrite.

@ Predictor X, Filter B

    add     r2, r12, #XfilterB
    ldmia   r2, {r2, r11}           @ r2 := p->XfilterB
                                    @ r11 := p->YfilterA

    rsb     r2, r2, r2, lsl #5      @ r2 := r2 * 32 - r2 ( == r2*31)
    sub     r10, r11, r2, asr #5    @ r10 (p->buf[XDELAYB]) := r11 - (r2 >> 5)

    str     r11, [r12, #XfilterB]   @ p->XfilterB := r11 (p->YfilterA)

    add     r11, r14, #XDELAYB-16   @ r11 := &p->buf[XDELAYB-4]

    ldmia   r11, { r2 - r5 }        @ r2 := p->buf[XDELAYB-4]
                                    @ r3 := p->buf[XDELAYB-3]
                                    @ r4 := p->buf[XDELAYB-2]
                                    @ r5 := p->buf[XDELAYB-1]

    subs     r5, r10, r5             @ r5 := r10 - r5

    add     r1, r12, #XcoeffsB
    ldmia   r1, {r6,r7,r8,r9,r11}   @ r6 := p->XcoeffsB[0]
                                    @ r7 := p->XcoeffsB[1]
                                    @ r8 := p->XcoeffsB[2]
                                    @ r9 := p->XcoeffsB[3]
                                    @ r11 := p->XcoeffsB[4]

    mul     r1, r10, r6             @ r1 := p->buf[XDELAYB] * p->XcoeffsB[0]
    mla     r1, r5, r7, r1          @ r1 += p->buf[XDELAYB-1] * p->XcoeffsB[1]
    mla     r1, r4, r8, r1          @ r1 += p->buf[XDELAYB-2] * p->XcoeffsB[2]
    mla     r1, r3, r9, r1          @ r1 += p->buf[XDELAYB-3] * p->XcoeffsB[3]
    mla     r1, r2, r11, r1         @ r1 += p->buf[XDELAYB-4] * p->XcoeffsB[4]

    add     r2, r14, #XDELAYB-4     @ r2 := &p->buf[XDELAYB-1]
    stmia   r2, { r5, r10 }         @ p->buf[XDELAYB-1] = r5
                                    @ p->buf[XDELAYB] = r10

    @ flags were set above, in the subs instruction
    mvngt   r5, #0
    movlt   r5, #1                 @ r5 := SIGN(r5) (see .c for SIGN macro)

    cmp     r10, #0
    mvngt   r10, #0
    movlt   r10, #1                @ r10 := SIGN(r10) (see .c for SIGN macro)

    add     r2, r14, #XADAPTCOEFFSB-4
    stmia   r2, {r5, r10}           @ p->buf[XADAPTCOEFFSB-1] := r5
                                    @ p->buf[XADAPTCOEFFSB] := r10

    @ r0 still contains predictionA
    @ r1 contains predictionB

    @ Finish Predictor X

    ldr     r2, [sp, #4]            @ r2 := decoded1
    add     r0, r0, r1, asr #1      @ r0 := r0 + (r1 >> 1)
    ldr     r3, [r2]                @ r3 := *decoded1
    add     r1, r3, r0, asr #10     @ r1 := r3 + (r0 >> 10)
    str     r1, [r12, #XlastA]      @ p->XlastA := r1

    ldr     r4, [r12, #XfilterA]    @ r4 := p->XfilterA
    rsb     r4, r4, r4, lsl #5      @ r4 := r4 * 32 - r4 ( == r4*31)
    add     r1, r1, r4, asr #5      @ r1 := r1 + (r4 >> 5)
    str     r1, [r12, #XfilterA]    @ p->XfilterA := r1

    @ r1 contains p->XfilterA
    @ r2 contains decoded1
    @ r3 contains *decoded1

    @ r6, r7, r8, r9, r11 contain p->XcoeffsB[0..4]
    @ r5, r10 contain p->buf[XADAPTCOEFFSB-1] and p->buf[XADAPTCOEFFSB]

    cmp     r3, #0
    stmia   r2!, {r1}               @ *(decoded1++) := r1  (p->XfilterA)
    str     r2, [sp, #4]            @ save decoded1
    beq     2f

    add     r1, r14, #XADAPTCOEFFSB-16
    ldmia   r1, { r2, r3, r4 }      @ r2 := p->buf[XADAPTCOEFFSB-4]
                                    @ r3 := p->buf[XADAPTCOEFFSB-3]
                                    @ r4 := p->buf[XADAPTCOEFFSB-2]
    blt     1f

    @ *decoded1 > 0

    sub     r6, r6, r10       @ r6 := p->XcoeffsB[0] - p->buf[XADAPTCOEFFSB]
    sub     r7, r7, r5        @ r7 := p->XcoeffsB[1] - p->buf[XADAPTCOEFFSB-1]
    sub     r8, r8, r4        @ r8 := p->XcoeffsB[2] - p->buf[XADAPTCOEFFSB-2]
    sub     r9, r9, r3        @ r9 := p->XcoeffsB[3] - p->buf[XADAPTCOEFFSB-3]
    sub     r11, r11, r2            @ r11 := p->XcoeffsB[4] - p->buf[XADAPTCOEFFSB-4]

    add     r0, r12, #XcoeffsB      
    stmia   r0, {r6,r7,r8,r9,r11}   @ Save p->XcoeffsB[]

    add     r1, r12, #XcoeffsA
    ldmia   r1, { r2-r5 }           @ r2 := p->XcoeffsA[0]
                                    @ r3 := p->XcoeffsA[1]
                                    @ r4 := p->XcoeffsA[2]
                                    @ r5 := p->XcoeffsA[3]

    add     r0, r14, #XADAPTCOEFFSA-12
    ldmia   r0, { r6-r9}            @ r6 := p->buf[XADAPTCOEFFSA-3]
                                    @ r7 := p->buf[XADAPTCOEFFSA-2]
                                    @ r8 := p->buf[XADAPTCOEFFSA-1]
                                    @ r9 := p->buf[XADAPTCOEFFSA]

    sub     r2, r2, r9        @ r2 := p->XcoeffsA[0] - p->buf[XADAPTCOEFFSA]
    sub     r3, r3, r8        @ r3 := p->XcoeffsA[1] - p->buf[XADAPTCOEFFSA-1]
    sub     r4, r4, r7        @ r4 := p->XcoeffsA[2] - p->buf[XADAPTCOEFFSA-2]
    sub     r5, r5, r6        @ r5 := p->XcoeffsA[3] - p->buf[XADAPTCOEFFSA-3]
    
    stmia   r1, {r2-r5}             @ Save p->XcoeffsA
    b       2f


1:  @ *decoded1 < 0

    add     r6, r6, r10       @ r6 := p->XcoeffsB[0] + p->buf[XADAPTCOEFFSB]
    add     r7, r7, r5        @ r7 := p->XcoeffsB[1] + p->buf[XADAPTCOEFFSB-1]
    add     r8, r8, r4        @ r8 := p->XcoeffsB[2] + p->buf[XADAPTCOEFFSB-2]
    add     r9, r9, r3        @ r9 := p->XcoeffsB[3] + p->buf[XADAPTCOEFFSB-3]
    add     r11, r11, r2      @ r11 := p->XcoeffsB[4] + p->buf[XADAPTCOEFFSB-4]

    add     r0, r12, #XcoeffsB      
    stmia   r0, {r6,r7,r8,r9,r11}   @ Save p->XcoeffsB[]

    add     r1, r12, #XcoeffsA
    ldmia   r1, { r2-r5 }           @ r2 := p->XcoeffsA[0]
                                    @ r3 := p->XcoeffsA[1]
                                    @ r4 := p->XcoeffsA[2]
                                    @ r5 := p->XcoeffsA[3]

    add     r0, r14, #XADAPTCOEFFSA-12
    ldmia   r0, { r6-r9}            @ r6 := p->buf[XADAPTCOEFFSA-3]
                                    @ r7 := p->buf[XADAPTCOEFFSA-2]
                                    @ r8 := p->buf[XADAPTCOEFFSA-1]
                                    @ r9 := p->buf[XADAPTCOEFFSA]

    add     r2, r2, r9        @ r2 := p->XcoeffsA[0] + p->buf[XADAPTCOEFFSA]
    add     r3, r3, r8        @ r3 := p->XcoeffsA[1] + p->buf[XADAPTCOEFFSA-1]
    add     r4, r4, r7        @ r4 := p->XcoeffsA[2] + p->buf[XADAPTCOEFFSA-2]
    add     r5, r5, r6        @ r5 := p->XcoeffsA[3] + p->buf[XADAPTCOEFFSA-3]
    
    stmia   r1, {r2-r5}             @ Save p->XcoeffsA

2:
    
@@@@@@@@@@@@@@@@@@@@@@@@@@@ COMMON

    add     r14, r14, #4                @ p->buf++

    add     r11, r12, #historybuffer    @ r11 := &p->historybuffer[0]

    sub     r10, r14, #PREDICTOR_HISTORY_SIZE*4
                                       @ r10 := p->buf - PREDICTOR_HISTORY_SIZE

    cmp     r10, r11
    beq     move_hist     @ The history buffer is full, we need to do a memmove

    @ Check loop count
    ldr     r0, [sp, #8]
    subs    r0, r0, #1
    strne   r0, [sp, #8]
    bne     loop

done:
    str     r14, [r12]              @ Save value of p->buf
    add     sp, sp, #12             @ Don't bother restoring r1-r3 
    ldmia   sp!, {r4-r11, pc}

move_hist:
    @ dest = r11 (p->historybuffer)
    @ src = r14 (p->buf)
    @ n = 200

    ldmia   r14!, {r0-r9}    @ 40 bytes
    stmia   r11!, {r0-r9}
    ldmia   r14!, {r0-r9}    @ 40 bytes
    stmia   r11!, {r0-r9}
    ldmia   r14!, {r0-r9}    @ 40 bytes
    stmia   r11!, {r0-r9}
    ldmia   r14!, {r0-r9}    @ 40 bytes
    stmia   r11!, {r0-r9}
    ldmia   r14!, {r0-r9}    @ 40 bytes
    stmia   r11!, {r0-r9}

    add     r14, r12, #historybuffer    @ p->buf = &p->historybuffer[0]

    @ Check loop count
    ldr     r0, [sp, #8]
    subs    r0, r0, #1
    strne   r0, [sp, #8]
    bne     loop
    
    b       done
