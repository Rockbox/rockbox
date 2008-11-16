/*

libdemac - A Monkey's Audio decoder

$Id$

Copyright (C) Dave Chapman 2007

Coldfire predictor copyright (C) 2007 Jens Arnold

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

    .text

    .align  2

    .global predictor_decode_stereo
    .type   predictor_decode_stereo,@function

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


| void predictor_decode_stereo(struct predictor_t* p,
|                              int32_t* decoded0,
|                              int32_t* decoded1,
|                              int count)

predictor_decode_stereo:
    lea.l   (-14*4,%sp), %sp
    movem.l %d2-%d7/%a2-%a6, (3*4,%sp)

    movem.l (14*4+8,%sp), %d0-%d2
    movem.l %d0-%d2, (%sp)              | (%sp) = decoded0
                                        | (4,%sp) = decoded1
                                        | (8,%sp) = count

    move.l  #0, %macsr                  | signed integer mode
    move.l  (14*4+4,%sp), %a6           | %a6 = p
    move.l  (%a6), %a5                  | %a5 = p->buf
    
.loop:
    
    | ***** PREDICTOR Y *****
    
    | Predictor Y, Filter A
    
    move.l  (YlastA,%a6), %d3           | %d3  = p->YlastA

    movem.l (YDELAYA-12,%a5), %d0-%d2   | %d0  = p->buf[YDELAYA-3]
                                        | %d1  = p->buf[YDELAYA-2]
                                        | %d2  = p->buf[YDELAYA-1]

    sub.l   %d3, %d2
    neg.l   %d2                         | %d2 = %d3 - %d2

    move.l  %d2, (YDELAYA-4,%a5)        | p->buf[YDELAYA-1]  = %d2

    movem.l (YcoeffsA,%a6), %a0-%a3     | %a0  = p->YcoeffsA[0]
                                        | %a1  = p->YcoeffsA[1]
                                        | %a2  = p->YcoeffsA[2]
                                        | %a3  = p->YcoeffsA[3]

    mac.l   %d3, %a0, %acc0             | %acc0  = p->buf[YDELAYA] * p->YcoeffsA[0]
    mac.l   %d2, %a1, %acc0             | %acc0 += p->buf[YDELAYA-1] * p->YcoeffsA[1]
    mac.l   %d1, %a2, %acc0             | %acc0 += p->buf[YDELAYA-2] * p->YcoeffsA[2]
    mac.l   %d0, %a3, %acc0             | %acc0 += p->buf[YDELAYA-3] * p->YcoeffsA[3]

    move.l  %d3, (YDELAYA,%a5)          | p->buf[YDELAYA]  = %d3

    tst.l   %d2
    beq.s   1f
    spl.b   %d2                         | pos: 0x??????ff, neg: 0x??????00
    extb.l  %d2                         | pos: 0xffffffff, neg: 0x00000000
    or.l    #1, %d2                     | pos: 0xffffffff, neg: 0x00000001
1:                                      | %d2 = SIGN(%d2)
    move.l  %d2, (YADAPTCOEFFSA-4,%a5)  | p->buf[YADAPTCOEFFSA-1]  = %d2

    tst.l   %d3
    beq.s   1f
    spl.b   %d3
    extb.l  %d3
    or.l    #1, %d3
1:                                      | %d3 = SIGN(%d3)
    move.l  %d3, (YADAPTCOEFFSA,%a5)    | p->buf[YADAPTCOEFFSA]  = %d3

    movclr.l %acc0, %d0

    | NOTE: %d0 now contains predictionA - don't overwrite.
    
    | Predictor Y, Filter B
    
    movem.l (YfilterB,%a6), %d2-%d3     | %d2  = p->YfilterB
                                        | %d3  = p->XfilterA
    move.l  %d3, (YfilterB,%a6)         | p->YfilterB = %d3

    move.l  %d2, %d1                    | %d1  = %d2
    lsl.l   #5, %d2                     | %d2  = %d2 * 32
    sub.l   %d1, %d2                    | %d2 -= %d1 (== 31 * old_d2)
    asr.l   #5, %d2                     | %d2 >>= 5
    sub.l   %d2, %d3                    | %d3 -= %d2
    
    movem.l (YDELAYB-16,%a5), %d4-%d7   | %d4  = p->buf[YDELAYB-4]
                                        | %d5  = p->buf[YDELAYB-3]
                                        | %d6  = p->buf[YDELAYB-2]
                                        | %d7  = p->buf[YDELAYB-1]
    sub.l   %d3, %d7
    neg.l   %d7                         | %d7  = %d3 - %d7

    move.l  %d7, (YDELAYB-4,%a5)        | p->buf[YDELAYB-1]  = %d7

    movem.l (YcoeffsB,%a6), %d2/%a0-%a3 | %d2  = p->YcoeffsB[0]
                                        | %a0  = p->YcoeffsB[1]
                                        | %a1  = p->YcoeffsB[2]
                                        | %a2  = p->YcoeffsB[3]
                                        | %a3  = p->YcoeffsB[4]

    mac.l   %d3, %d2, %acc0             | %acc0  = p->buf[YDELAYB] * p->YcoeffsB[0]
    mac.l   %d7, %a0, %acc0             | %acc0 += p->buf[YDELAYB-1] * p->YcoeffsB[1]
    mac.l   %d6, %a1, %acc0             | %acc0 += p->buf[YDELAYB-2] * p->YcoeffsB[2]
    mac.l   %d5, %a2, %acc0             | %acc0 += p->buf[YDELAYB-3] * p->YcoeffsB[3]
    mac.l   %d4, %a3, %acc0             | %acc0 += p->buf[YDELAYB-4] * p->YcoeffsB[4]
    
    move.l  %d3, (YDELAYB, %a5)         | p->buf[YDELAYB]  = %d3
    
    tst.l   %d7
    beq.s   1f
    spl.b   %d7
    extb.l  %d7
    or.l    #1, %d7
1:                                      | %d7 = SIGN(%d7)
    move.l  %d7, (YADAPTCOEFFSB-4,%a5)  | p->buf[YADAPTCOEFFSB-1]  = %d7
    tst.l   %d3
    beq.s   1f
    spl.b   %d3
    extb.l  %d3
    or.l    #1, %d3
1:                                      | %d3 = SIGN(%d3)
    move.l  %d3, (YADAPTCOEFFSB, %a5)   | p->buf[YADAPTCOEFFSB]  = %d3

    movclr.l %acc0, %d1
    
    | %d0 still contains predictionA
    | %d1 contains predictionB

    | Finish Predictor Y

    move.l  (%sp), %a4                  | %a4  = decoded0
    asr.l   #1, %d1
    add.l   %d1, %d0                    | %d0 += (%d1 >> 1)
    move.l  (%a4), %d5                  | %d5  = *decoded0
    move.l  %d5, %d1                    | %d1  = %d5
    asr.l   #8, %d0
    asr.l   #2, %d0                     | %d0 >>= 10
    add.l   %d0, %d1                    | %d1 += %d0
    move.l  %d1, (YlastA,%a6)           | p->YlastA  = %d1
    
    move.l  (YfilterA,%a6), %d6         | %d6  = p->YfilterA
    move.l  %d6, %d0
    lsl.l   #5, %d6
    sub.l   %d0, %d6                    | %d6 = 31 * %d6
    asr.l   #5, %d6                     | %d6 >>= 5
    add.l   %d6, %d1
    move.l  %d1, (YfilterA,%a6)         | p->YfilterA  = %d1

    | %d1 contains p->YfilterA
    | %a4 contains decoded0
    | %d5 contains *decoded0

    | %d2, %a0, %a1, %a2, %a3 contain p->YcoeffsB[0..4]
    | %d7, %d3 contain p->buf[YADAPTCOEFFSB-1] and p->buf[YADAPTCOEFFSB]

    move.l  %d1, (%a4)+                 | *(decoded0++)  = %d1 (p->YfilterA)
    move.l  %a4, (%sp)                  | save decoded0    
    tst.l   %d5
    beq.s   3f

    movem.l (YADAPTCOEFFSB-16,%a5), %d4-%d6 | %d4  = p->buf[YADAPTCOEFFSB-4]
                                            | %d5  = p->buf[YADAPTCOEFFSB-3]
                                            | %d6  = p->buf[YADAPTCOEFFSB-2]

    bmi.s   1f                          | flags still valid here

    | *decoded0 > 0

    sub.l   %d3, %d2                    | %d2  = p->YcoeffsB[0] - p->buf[YADAPTCOEFFSB]
    sub.l   %d7, %a0                    | %a0  = p->YcoeffsB[1] - p->buf[YADAPTCOEFFSB-1]
    sub.l   %d6, %a1                    | %a1  = p->YcoeffsB[2] - p->buf[YADAPTCOEFFSB-2]
    sub.l   %d5, %a2                    | %a2  = p->YcoeffsB[3] - p->buf[YADAPTCOEFFSB-3]
    sub.l   %d4, %a3                    | %a3  = p->YcoeffsB[4] - p->buf[YADAPTCOEFFSB-4]
    
    movem.l %d2/%a0-%a3, (YcoeffsB,%a6) | Save p->YcoeffsB[]
    
    movem.l (YcoeffsA,%a6), %d4-%d7     | %d4  = p->YcoeffsA[0]
                                        | %d5  = p->YcoeffsA[1]
                                        | %d6  = p->YcoeffsA[2]
                                        | %d7  = p->YcoeffsA[3]
                                        
    movem.l (YADAPTCOEFFSA-12,%a5), %d2/%a0-%a2 | %d2  = p->buf[YADAPTCOEFFSA-3]
                                        | %a0  = p->buf[YADAPTCOEFFSA-2]
                                        | %a1  = p->buf[YADAPTCOEFFSA-1]
                                        | %a2  = p->buf[YADAPTCOEFFSA]
                                        
    sub.l   %a2, %d4                    | %d4  = p->YcoeffsA[0] - p->buf[YADAPTCOEFFSA]
    sub.l   %a1, %d5                    | %d5  = p->YcoeffsA[1] - p->buf[YADAPTCOEFFSA-1]
    sub.l   %a0, %d6                    | %d6  = p->YcoeffsA[2] - p->buf[YADAPTCOEFFSA-2]
    sub.l   %d2, %d7                    | %d7  = p->YcoeffsA[3] - p->buf[YADAPTCOEFFSA-3]
    
    bra.s   2f

1:  | *decoded0 < 0

    add.l   %d3, %d2                    | %d2  = p->YcoeffsB[0] + p->buf[YADAPTCOEFFSB]
    add.l   %d7, %a0                    | %a0  = p->YcoeffsB[1] + p->buf[YADAPTCOEFFSB-1]
    add.l   %d6, %a1                    | %a1  = p->YcoeffsB[2] + p->buf[YADAPTCOEFFSB-2]
    add.l   %d5, %a2                    | %a2  = p->YcoeffsB[3] + p->buf[YADAPTCOEFFSB-3]
    add.l   %d4, %a3                    | %a3  = p->YcoeffsB[4] + p->buf[YADAPTCOEFFSB-4]
    
    movem.l %d2/%a0-%a3, (YcoeffsB,%a6) | Save p->YcoeffsB[]
    
    movem.l (YcoeffsA,%a6), %d4-%d7     | %d4  = p->YcoeffsA[0]
                                        | %d5  = p->YcoeffsA[1]
                                        | %d6  = p->YcoeffsA[2]
                                        | %d7  = p->YcoeffsA[3]
                                        
    movem.l (YADAPTCOEFFSA-12,%a5), %d2/%a0-%a2 | %d2  = p->buf[YADAPTCOEFFSA-3]
                                        | %a0  = p->buf[YADAPTCOEFFSA-2]
                                        | %a1  = p->buf[YADAPTCOEFFSA-1]
                                        | %a2  = p->buf[YADAPTCOEFFSA]
                                        
    add.l   %a2, %d4                    | %d4  = p->YcoeffsA[0] + p->buf[YADAPTCOEFFSA]
    add.l   %a1, %d5                    | %d5  = p->YcoeffsA[1] + p->buf[YADAPTCOEFFSA-1]
    add.l   %a0, %d6                    | %d6  = p->YcoeffsA[2] + p->buf[YADAPTCOEFFSA-2]
    add.l   %d2, %d7                    | %d7  = p->YcoeffsA[3] + p->buf[YADAPTCOEFFSA-3]

2:
    movem.l %d4-%d7, (YcoeffsA,%a6)     | Save p->YcoeffsA[]

3:

    | ***** PREDICTOR X *****
    
    | Predictor X, Filter A
    
    move.l  (XlastA,%a6), %d3           | %d3  = p->XlastA

    movem.l (XDELAYA-12,%a5), %d0-%d2   | %d0  = p->buf[XDELAYA-3]
                                        | %d1  = p->buf[XDELAYA-2]
                                        | %d2  = p->buf[XDELAYA-1]

    sub.l   %d3, %d2                    
    neg.l   %d2                         | %d2  = %d3 -%d2

    move.l  %d2, (XDELAYA-4,%a5)        | p->buf[XDELAYA-1]  = %d2

    movem.l (XcoeffsA,%a6), %a0-%a3     | %a0  = p->XcoeffsA[0]
                                        | %a1  = p->XcoeffsA[1]
                                        | %a2  = p->XcoeffsA[2]
                                        | %a3  = p->XcoeffsA[3]

    mac.l   %d3, %a0, %acc0             | %acc0  = p->buf[XDELAYA] * p->XcoeffsA[0]
    mac.l   %d2, %a1, %acc0             | %acc0 += p->buf[XDELAYA-1] * p->XcoeffsA[1]
    mac.l   %d1, %a2, %acc0             | %acc0 += p->buf[XDELAYA-2] * p->XcoeffsA[2]
    mac.l   %d0, %a3, %acc0             | %acc0 += p->buf[XDELAYA-3] * p->XcoeffsA[3]

    move.l  %d3, (XDELAYA,%a5)          | p->buf[XDELAYA]  = %d3
    
    tst.l   %d2
    beq.s   1f
    spl.b   %d2                         | pos: 0x??????ff, neg: 0x??????00
    extb.l  %d2                         | pos: 0xffffffff, neg: 0x00000000
    or.l    #1, %d2                     | pos: 0xffffffff, neg: 0x00000001
1:                                      | %d2 = SIGN(%d2)
    move.l  %d2, (XADAPTCOEFFSA-4,%a5)  | p->buf[XADAPTCOEFFSA-1]  = %d2

    tst.l   %d3
    beq.s   1f
    spl.b   %d3
    extb.l  %d3
    or.l    #1, %d3
1:                                      | %d3 = SIGN(%d3)
    move.l  %d3, (XADAPTCOEFFSA,%a5)    | p->buf[XADAPTCOEFFSA]  = %d3

    movclr.l %acc0, %d0

    | NOTE: %d0 now contains predictionA - don't overwrite.
    
    | Predictor X, Filter B
    
    movem.l (XfilterB,%a6), %d2-%d3     | %d2  = p->XfilterB
                                        | %d3  = p->YfilterA
    move.l  %d3, (XfilterB,%a6)         | p->XfilterB = %d3

    move.l  %d2, %d1                    | %d1  = %d2
    lsl.l   #5, %d2                     | %d2  = %d2 * 32
    sub.l   %d1, %d2                    | %d2 -= %d1 (== 31 * old_d2)
    asr.l   #5, %d2                     | %d2 >>= 5
    sub.l   %d2, %d3                    | %d3 -= %d2 
    
    movem.l (XDELAYB-16,%a5), %d4-%d7   | %d4  = p->buf[XDELAYB-4]
                                        | %d5  = p->buf[XDELAYB-3]
                                        | %d6  = p->buf[XDELAYB-2]
                                        | %d7  = p->buf[XDELAYB-1]
    sub.l   %d3, %d7
    neg.l   %d7                         | %d7  = %d3 - %d7

    move.l  %d7, (XDELAYB-4,%a5)        | p->buf[XDELAYB-1]  = %d7

    movem.l (XcoeffsB,%a6), %d2/%a0-%a3 | %d2  = p->XcoeffsB[0]
                                        | %a0  = p->XcoeffsB[1]
                                        | %a1  = p->XcoeffsB[2]
                                        | %a2  = p->XcoeffsB[3]
                                        | %a3  = p->XcoeffsB[4]

    mac.l   %d3, %d2, %acc0             | %acc0  = p->buf[XDELAYB] * p->XcoeffsB[0]
    mac.l   %d7, %a0, %acc0             | %acc0 += p->buf[XDELAYB-1] * p->XcoeffsB[1]
    mac.l   %d6, %a1, %acc0             | %acc0 += p->buf[XDELAYB-2] * p->XcoeffsB[2]
    mac.l   %d5, %a2, %acc0             | %acc0 += p->buf[XDELAYB-3] * p->XcoeffsB[3]
    mac.l   %d4, %a3, %acc0             | %acc0 += p->buf[XDELAYB-4] * p->XcoeffsB[4]
    
    move.l  %d3, (XDELAYB, %a5)         | p->buf[XDELAYB]  = %d3
    
    tst.l   %d7
    beq.s   1f
    spl.b   %d7
    extb.l  %d7
    or.l    #1, %d7
1:                                      | %d7 = SIGN(%d7)
    move.l  %d7, (XADAPTCOEFFSB-4,%a5)  | p->buf[XADAPTCOEFFSB-1]  = %d7

    tst.l   %d3
    beq.s   1f
    spl.b   %d3
    extb.l  %d3
    or.l    #1, %d3
1:                                      | %d3 = SIGN(%d3)
    move.l  %d3, (XADAPTCOEFFSB, %a5)   | p->buf[XADAPTCOEFFSB]  = %d3

    movclr.l %acc0, %d1

    | %d0 still contains predictionA
    | %d1 contains predictionB

    | Finish Predictor X

    move.l  (4,%sp), %a4                | %a4  = decoded1
    asr.l   #1, %d1
    add.l   %d1, %d0                    | %d0 += (%d1 >> 1)
    move.l  (%a4), %d5                  | %d5  = *decoded1
    move.l  %d5, %d1                    | %d1 = %d5
    asr.l   #8, %d0
    asr.l   #2, %d0                     | %d0 >>= 10
    add.l   %d0, %d1                    | %d1 += %d0
    move.l  %d1, (XlastA,%a6)           | p->XlastA  = %d1
    
    move.l  (XfilterA,%a6), %d6         | %d6  = p->XfilterA
    move.l  %d6, %d0
    lsl.l   #5, %d6
    sub.l   %d0, %d6                    | %d6 = 31 * %d6
    asr.l   #5, %d6                     | %d6 >>= 5
    add.l   %d6, %d1
    move.l  %d1, (XfilterA,%a6)         | p->XfilterA  = %d6

    | %d1 contains p->XfilterA
    | %a4 contains decoded1
    | %d5 contains *decoded1

    | %d2, %a0, %a1, %a2, %a3 contain p->XcoeffsB[0..4]
    | %d7, %d3 contain p->buf[XADAPTCOEFFSB-1] and p->buf[XADAPTCOEFFSB]

    move.l  %d1, (%a4)+                 | *(decoded1++)  = %d1 (p->XfilterA)
    move.l  %a4, (4,%sp)                | save decoded1
    tst.l   %d5
    beq.s   3f
    
    movem.l (XADAPTCOEFFSB-16,%a5), %d4-%d6 | %d4  = p->buf[XADAPTCOEFFSB-4]
                                            | %d5  = p->buf[XADAPTCOEFFSB-3]
                                            | %d6  = p->buf[XADAPTCOEFFSB-2]

    bmi.s   1f                          | flags still valid here
    
    | *decoded1 > 0

    sub.l   %d3, %d2                    | %d2  = p->XcoeffsB[0] - p->buf[XADAPTCOEFFSB]
    sub.l   %d7, %a0                    | %a0  = p->XcoeffsB[1] - p->buf[XADAPTCOEFFSB-1]
    sub.l   %d6, %a1                    | %a1  = p->XcoeffsB[2] - p->buf[XADAPTCOEFFSB-2]
    sub.l   %d5, %a2                    | %a2  = p->XcoeffsB[3] - p->buf[XADAPTCOEFFSB-3]
    sub.l   %d4, %a3                    | %a3  = p->XcoeffsB[4] - p->buf[XADAPTCOEFFSB-4]
    
    movem.l %d2/%a0-%a3, (XcoeffsB,%a6) | Save p->XcoeffsB[]
    
    movem.l (XcoeffsA,%a6), %d4-%d7     | %d4  = p->XcoeffsA[0]
                                        | %d5  = p->XcoeffsA[1]
                                        | %d6  = p->XcoeffsA[2]
                                        | %d7  = p->XcoeffsA[3]
                                        
    movem.l (XADAPTCOEFFSA-12,%a5), %d2/%a0-%a2 | %d2  = p->buf[XADAPTCOEFFSA-3]
                                        | %a0  = p->buf[XADAPTCOEFFSA-2]
                                        | %a1  = p->buf[XADAPTCOEFFSA-1]
                                        | %a2  = p->buf[XADAPTCOEFFSA]
                                        
    sub.l   %a2, %d4                    | %d4  = p->XcoeffsA[0] - p->buf[XADAPTCOEFFSA]
    sub.l   %a1, %d5                    | %d5  = p->XcoeffsA[1] - p->buf[XADAPTCOEFFSA-1]
    sub.l   %a0, %d6                    | %d6  = p->XcoeffsA[2] - p->buf[XADAPTCOEFFSA-2]
    sub.l   %d2, %d7                    | %d7  = p->XcoeffsA[3] - p->buf[XADAPTCOEFFSA-3]
    
    bra.s   2f

1:  | *decoded1 < 0

    add.l   %d3, %d2                    | %d2  = p->XcoeffsB[0] + p->buf[XADAPTCOEFFSB]
    add.l   %d7, %a0                    | %a0  = p->XcoeffsB[1] + p->buf[XADAPTCOEFFSB-1]
    add.l   %d6, %a1                    | %a1  = p->XcoeffsB[2] + p->buf[XADAPTCOEFFSB-2]
    add.l   %d5, %a2                    | %a2  = p->XcoeffsB[3] + p->buf[XADAPTCOEFFSB-3]
    add.l   %d4, %a3                    | %a3  = p->XcoeffsB[4] + p->buf[XADAPTCOEFFSB-4]
    
    movem.l %d2/%a0-%a3, (XcoeffsB,%a6) | Save p->XcoeffsB[]
    
    movem.l (XcoeffsA,%a6), %d4-%d7     | %d4  = p->XcoeffsA[0]
                                        | %d5  = p->XcoeffsA[1]
                                        | %d6  = p->XcoeffsA[2]
                                        | %d7  = p->XcoeffsA[3]

    movem.l (XADAPTCOEFFSA-12,%a5), %d2/%a0-%a2 | %d2  = p->buf[XADAPTCOEFFSA-3]
                                        | %a0  = p->buf[XADAPTCOEFFSA-2]
                                        | %a1  = p->buf[XADAPTCOEFFSA-1]
                                        | %a2  = p->buf[XADAPTCOEFFSA]
                                        
    add.l   %a2, %d4                    | %d4  = p->XcoeffsA[0] + p->buf[XADAPTCOEFFSA]
    add.l   %a1, %d5                    | %d5  = p->XcoeffsA[1] + p->buf[XADAPTCOEFFSA-1]
    add.l   %a0, %d6                    | %d6  = p->XcoeffsA[2] + p->buf[XADAPTCOEFFSA-2]
    add.l   %d2, %d7                    | %d7  = p->XcoeffsA[3] + p->buf[XADAPTCOEFFSA-3]

2:
    movem.l %d4-%d7, (XcoeffsA,%a6)     | Save p->XcoeffsA[]

3:

    | ***** COMMON *****
    
    addq.l  #4, %a5                     | p->buf++

    lea.l   (historybuffer+PREDICTOR_HISTORY_SIZE*4,%a6), %a3 
                              | %a3 = &p->historybuffer[PREDICTOR_HISTORY_SIZE]
    
    cmp.l   %a3, %a5                    
    bne.s   .endofloop
                                              
    | The history buffer is full, we need to do a memmove:

    lea.l   (historybuffer,%a6), %a3    

    | dest = %a3 (p->historybuffer)
    | src = %a5 (p->buf)
    | n = 200
    
    movem.l (%a5), %d0-%d7/%a0-%a1      | 40 bytes
    lea.l   (40,%a5), %a5
    movem.l %d0-%d7/%a0-%a1, (%a3)
    lea.l   (40,%a3), %a3
    movem.l (%a5), %d0-%d7/%a0-%a1      | 40 bytes
    lea.l   (40,%a5), %a5
    movem.l %d0-%d7/%a0-%a1, (%a3)
    lea.l   (40,%a3), %a3
    movem.l (%a5), %d0-%d7/%a0-%a1      | 40 bytes
    lea.l   (40,%a5), %a5
    movem.l %d0-%d7/%a0-%a1, (%a3)
    lea.l   (40,%a3), %a3
    movem.l (%a5), %d0-%d7/%a0-%a1      | 40 bytes
    lea.l   (40,%a5), %a5
    movem.l %d0-%d7/%a0-%a1, (%a3)
    lea.l   (40,%a3), %a3
    movem.l (%a5), %d0-%d7/%a0-%a1      | 40 bytes
    lea.l   (40,%a5), %a5
    movem.l %d0-%d7/%a0-%a1, (%a3)
    lea.l   (40,%a3), %a3

    lea.l   (historybuffer,%a6), %a5    | p->buf = &p->historybuffer[0]

.endofloop:
    subq.l  #1, (8,%sp)                 | decrease loop count
    bne.w   .loop

    move.l  %a5, (%a6)                  | Save value of p->buf
    movem.l (3*4,%sp), %d2-%d7/%a2-%a6
    lea.l   (14*4,%sp), %sp
    rts
