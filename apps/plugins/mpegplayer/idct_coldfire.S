/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Jens Arnold
 * Based on the work of Karim Boucher and Rani Hod
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
    .global     mpeg2_idct_copy_coldfire
    .type       mpeg2_idct_copy_coldfire, @function
    .global     mpeg2_idct_add_coldfire
    .type       mpeg2_idct_add_coldfire, @function

    /* The IDCT itself.
     * Input: %a0: block pointer
     * Caller must save all registers. */
    .align  2
.idct:
    move.l  %a0, %a6

    move.l  #0, %macsr              | signed integer mode

    move.l  #((2048<<16)+2841), %a0 | W0,  W1
    move.l  #((2676<<16)+2408), %a1 | W2,  W3
    move.l  #((2048<<16)+1609), %a2 | W4,  W5
    move.l  #((1108<<16)+ 565), %a3 | W6,  W7
    
    lea.l   (128,%a6), %a4      | secondary, transposed temp buffer
    moveq.l #8, %d3             | loop counter
    
.row_loop:
    movem.l (%a6), %d0-%d2/%a5  | fetch (f0, f2, f4, f6, f1, f3, f5, f7)
    
    mac.w   %a0l, %d2u, %acc0   | %acc0 = W1 * f1
    mac.w   %a1l, %d2l, %acc0   |       + W3 * f3
    mac.w   %a2l, %a5u, %acc0   |       + W5 * f5
    mac.w   %a3l, %a5l, %acc0   |       + W7 * f7

    mac.w   %a1l, %d2u, %acc1   | %acc1 = W3 * f1
    msac.w  %a3l, %d2l, %acc1   |       - W7 * f3
    msac.w  %a0l, %a5u, %acc1   |       - W1 * f5
    msac.w  %a2l, %a5l, %acc1   |       - W5 * f7
    
    mac.w   %a2l, %d2u, %acc2   | %acc2 = W5 * f1
    msac.w  %a0l, %d2l, %acc2   |       - W1 * f3
    mac.w   %a3l, %a5u, %acc2   |       + W7 * f5
    mac.w   %a1l, %a5l, %acc2   |       + W3 * f7

    mac.w   %a3l, %d2u, %acc3   | %acc3 = W7 * f1
    msac.w  %a2l, %d2l, %acc3   |       - W5 * f3
    mac.w   %a1l, %a5u, %acc3   |       + W3 * f5
    msac.w  %a0l, %a5l, %acc3   |       - W1 * f7

    lea.l   (16,%a6), %a6       | Advance to next row; put here to fill EMAC latency
    add.l   #(1<<16), %d0       | f0 += 1; 

    movclr.l %acc0, %d4         | b0
    movclr.l %acc1, %d5         | b1
    movclr.l %acc2, %d6         | b2
    movclr.l %acc3, %d7         | b3

    mac.w   %a0u, %d0u, %acc0   | %acc0 = W0 * f0
    mac.w   %a2u, %d1u, %acc0   |       + W4 * f4
    move.l  %acc0, %acc3
    mac.w   %a1u, %d0l, %acc0   |       + W2 * f2
    mac.w   %a3u, %d1l, %acc0   |       + W6 * f6

    mac.w   %a0u, %d0u, %acc1   | %acc1 = W0 * f0
    msac.w  %a2u, %d1u, %acc1   |       - W4 * f4
    move.l  %acc1, %acc2
    mac.w   %a3u, %d0l, %acc1   |       + W6 * f2
    msac.w  %a1u, %d1l, %acc1   |       - W2 * f6

    | ^ move.l  %acc0, %acc3      %acc2 = W0 * f0 - W4 * f4
    msac.w  %a3u, %d0l, %acc2   |       - W6 * f2
    mac.w   %a1u, %d1l, %acc2   |       + W2 * f6

    | ^ move.l  %acc1, %acc2      %acc3 = W0 * f0 + W4 * f4
    msac.w  %a1u, %d0l, %acc3   |       - W2 * f2
    msac.w  %a3u, %d1l, %acc3   |       - W6 * f6

    moveq.l #12, %d1            | shift amount

    move.l  %acc0, %d0          | block[7] = (a0
    sub.l   %d4,%d0             |     - b0)
    asr.l   %d1, %d0            |     >> 12
    move.w  %d0, (7*16,%a4)

    move.l  %acc1, %d0          | block[6] = (a1
    sub.l   %d5,%d0             |     - b1)
    asr.l   %d1, %d0            |     >> 12
    move.w  %d0, (6*16,%a4)
   
    move.l  %acc2, %d0          | block[5] = (a2
    sub.l   %d6,%d0             |     - b2)
    asr.l   %d1, %d0            |     >> 12
    move.w  %d0, (5*16,%a4)
   
    move.l  %acc3, %d0          | block[4] = (a3
    sub.l   %d7,%d0             |     - b3)
    asr.l   %d1, %d0            |     >> 12
    move.w  %d0, (4*16,%a4)

    movclr.l %acc3, %d0         | block[3] = (a3
    add.l   %d7, %d0            |     + b3)
    asr.l   %d1, %d0            |     >> 12
    move.w  %d0, (3*16,%a4)

    movclr.l %acc2, %d0         | block[2] = (a2
    add.l   %d6, %d0            |     + b2)
    asr.l   %d1, %d0            |     >> 12
    move.w  %d0, (2*16,%a4)

    movclr.l %acc1, %d0         | block[1] = (a1
    add.l   %d5, %d0            |     + b1)
    asr.l   %d1, %d0            |     >> 12
    move.w  %d0, (1*16,%a4)

    movclr.l %acc0, %d0         | block[0] = (a0
    add.l   %d4, %d0            |     + b0)
    asr.l   %d1, %d0            |     >> 12
    move.w  %d0, (%a4)+         | advance to next temp column
    
    subq.l  #1, %d3             | loop 8 times
    bne.w   .row_loop
    
    | %a6 now points to the temp buffer, where we need it.
    lea.l   (-16-128,%a4), %a4  | point %a4 back to the input block
    moveq.l #8, %d3             | loop counter
    
.col_loop:
    movem.l (%a6), %d0-%d2/%a5  | fetch (f0, f2, f4, f6, f1, f3, f5, f7)
  
    mac.w   %a0l, %d2u, %acc0   | %acc0 = W1 * f1
    mac.w   %a1l, %d2l, %acc0   |       + W3 * f3
    mac.w   %a2l, %a5u, %acc0   |       + W5 * f5
    mac.w   %a3l, %a5l, %acc0   |       + W7 * f7

    mac.w   %a1l, %d2u, %acc1   | %acc1 = W3 * f1
    msac.w  %a3l, %d2l, %acc1   |       - W7 * f3
    msac.w  %a0l, %a5u, %acc1   |       - W1 * f5
    msac.w  %a2l, %a5l, %acc1   |       - W5 * f7
    
    mac.w   %a2l, %d2u, %acc2   | %acc2 = W5 * f1
    msac.w  %a0l, %d2l, %acc2   |       - W1 * f3
    mac.w   %a3l, %a5u, %acc2   |       + W7 * f5
    mac.w   %a1l, %a5l, %acc2   |       + W3 * f7

    mac.w   %a3l, %d2u, %acc3   | %acc3 = W7 * f1
    msac.w  %a2l, %d2l, %acc3   |       - W5 * f3
    mac.w   %a1l, %a5u, %acc3   |       + W3 * f5
    msac.w  %a0l, %a5l, %acc3   |       - W1 * f7
    
    lea.l   (16,%a6), %a6       | Advance to next row; put here to fill EMAC latency
    add.l   #(32<<16), %d0      | DC offset: 0.5

    movclr.l %acc0, %d4         | b0
    movclr.l %acc1, %d5         | b1
    movclr.l %acc2, %d6         | b2
    movclr.l %acc3, %d7         | b3

    mac.w   %a0u, %d0u, %acc0   | %acc0 = W0 * f0
    mac.w   %a2u, %d1u, %acc0   |       + W4 * f4
    move.l  %acc0, %acc3
    mac.w   %a1u, %d0l, %acc0   |       + W2 * f2
    mac.w   %a3u, %d1l, %acc0   |       + W6 * f6

    mac.w   %a0u, %d0u, %acc1   | %acc1 = W0 * f0
    msac.w  %a2u, %d1u, %acc1   |       - W4 * f4
    move.l  %acc1, %acc2
    mac.w   %a3u, %d0l, %acc1   |       + W6 * f2
    msac.w  %a1u, %d1l, %acc1   |       - W2 * f6

    | ^ move.l  %acc1, %acc2      %acc2 = W0 * f0 - W4 * f4
    msac.w  %a3u, %d0l, %acc2   |       - W6 * f2
    mac.w   %a1u, %d1l, %acc2   |       + W2 * f6

    | ^ move.l  %acc0, %acc3      %acc3 = W0 * f0 + W4 * f4
    msac.w  %a1u, %d0l, %acc3   |       - W2 * f2
    msac.w  %a3u, %d1l, %acc3   |       - W6 * f6

    moveq.l #17, %d1            | shift amount

    move.l  %acc0, %d0          | block[7] = (a0
    sub.l   %d4,%d0             |     - b0)
    asr.l   %d1, %d0            |     >> 17
    move.w  %d0, (7*16,%a4)
   
    move.l  %acc1, %d0          | block[6] = (a1
    sub.l   %d5,%d0             |     - b1)
    asr.l   %d1, %d0            |     >> 17
    move.w  %d0, (6*16,%a4)
   
    move.l  %acc2, %d0          | block[5] = (a2
    sub.l   %d6,%d0             |     - b2)
    asr.l   %d1, %d0            |     >> 17
    move.w  %d0, (5*16,%a4)
   
    move.l  %acc3, %d0          | block[4] = (a3
    sub.l   %d7,%d0             |     - b3)
    asr.l   %d1, %d0            |     >> 17
    move.w  %d0, (4*16,%a4)
   
    movclr.l %acc3, %d0         | block[3] = (a3
    add.l   %d7, %d0            |     + b3)
    asr.l   %d1, %d0            |     >> 17
    move.w  %d0, (3*16,%a4)
   
    movclr.l %acc2, %d0         | block[2] = (a2
    add.l   %d6, %d0            |     + b2)
    asr.l   %d1, %d0            |     >> 17
    move.w  %d0, (2*16,%a4)
   
    movclr.l %acc1, %d0         | block[1] = (a1
    add.l   %d5, %d0            |     + b1)
    asr.l   %d1, %d0            |     >> 17
    move.w  %d0, (1*16,%a4)
   
    movclr.l %acc0, %d0         | block[0] = (a0
    add.l   %d4, %d0            |     + b0)
    asr.l   %d1, %d0            |     >> 17
    move.w  %d0, (%a4)+         | advance to next column

    subq.l  #1, %d3             | loop 8 times
    bne.w   .col_loop
    
    rts
    
    .align  2

mpeg2_idct_copy_coldfire:
    lea.l   (-11*4,%sp), %sp
    movem.l %d2-%d7/%a2-%a6, (%sp)  | save some registers
    move.l  (11*4+4,%sp), %a0       | %a0 - block pointer for idct

    bsr.w   .idct                   | apply idct to block
    movem.l (11*4+4,%sp), %a0-%a2   | %a0 - block pointer
                                    | %a1 - destination pointer
                                    | %a2 - stride

    move.l  #255, %d1           | preload constant for clipping
    moveq.l #8, %d4             | loop counter
    
.copy_clip_loop:
    move.w  (%a0), %d0          | load block[0]
    ext.l   %d0                 | sign extend
    cmp.l   %d1, %d0            | overflow?
    bls.b   1f
    spl.b   %d0                 |   yes: set appropriate limit value in low byte
1:
    move.b  %d0, %d2            | collect output bytes 0..3 in %d2
    lsl.l   #8, %d2

    move.w  (2,%a0), %d0        | load block[1]
    ext.l   %d0                 | sign extend
    cmp.l   %d1, %d0            | overflow?
    bls.b   1f
    spl.b   %d0                 | yes: set appropriate limit value in low byte
1:
    move.b  %d0, %d2            | collect output bytes 0..3 in %d2
    lsl.l   #8, %d2
    clr.l   (%a0)+              | clear block[0] and block[1],
                                | %a0 now pointing to block[2]
    move.w  (%a0), %d0          | do b2 and b3
    ext.l   %d0
    cmp.l   %d1, %d0
    bls.b   1f
    spl.b   %d0
1:
    move.b  %d0, %d2
    lsl.l   #8, %d2

    move.w  (2,%a0), %d0
    ext.l   %d0
    cmp.l   %d1, %d0
    bls.b   1f
    spl.b   %d0
1:
    move.b  %d0, %d2
    clr.l   (%a0)+

    move.w  (%a0), %d0          | do b4 and b5
    ext.l   %d0
    cmp.l   %d1, %d0
    bls.b   1f
    spl.b   %d0
1:
    move.b  %d0, %d3
    lsl.l   #8, %d3

    move.w  (2,%a0), %d0
    ext.l   %d0
    cmp.l   %d1, %d0
    bls.b   1f
    spl.b   %d0
1:
    move.b  %d0, %d3
    lsl.l   #8, %d3
    clr.l   (%a0)+

    move.w  (%a0), %d0          | do b6 and b7
    ext.l   %d0
    cmp.l   %d1, %d0
    bls.b   1f
    spl.b   %d0
1:
    move.b  %d0, %d3
    lsl.l   #8, %d3

    move.w  (2,%a0), %d0
    ext.l   %d0
    cmp.l   %d1, %d0
    bls.b   1f
    spl.b   %d0
1:
    move.b  %d0, %d3            
    clr.l   (%a0)+
    
    movem.l %d2-%d3, (%a1)      | write all 8 output bytes at once
    lea.l   (%a2,%a1), %a1      | advance output pointer
    subq.l  #1, %d4             | loop 8 times
    bne.w   .copy_clip_loop

    movem.l (%sp), %d2-%d7/%a2-%a6
    lea.l   (11*4,%sp), %sp
    rts

    .align  2

mpeg2_idct_add_coldfire:
    lea.l   (-11*4,%sp), %sp
    movem.l %d2-%d7/%a2-%a6, (%sp)
    movem.l (11*4+4,%sp), %d0/%a0-%a2   | %d0 - last value
                                        | %a0 - block pointer
                                        | %a1 - destination pointer
                                        | %a2 - stride

    cmp.l   #129, %d0           | last == 129 ?
    bne.b   .idct_add           |   no: perform idct + addition
    move.w  (%a0), %d0
    ext.l   %d0                 | ((block[0]
    asr.l   #4, %d0             |      >> 4)
    and.l   #7, %d0             |      & 7)
    subq.l  #4, %d0             |      - 4 == 0 ?
    bne.w   .dc_add             |   no: just perform addition

.idct_add:
    bsr.w   .idct                   | apply idct
    movem.l (11*4+8,%sp), %a0-%a2   | reload arguments %a0..%a2

    move.l  #255, %d2           | preload constant for clipping
    clr.l   %d3                 | used for splitting input words into bytes
    moveq.l #8, %d4             | loop counter
    
.add_clip_loop:
    movem.l (%a1), %d6-%d7      | fetch (b0 b1 b2 b3) (b4 b5 b6 b7)
    swap    %d6                 | (b2 b3 b0 b1) 
    swap    %d7                 | (b6 b7 b4 b5)
    
    move.w  (2,%a0), %d0        | load block[1]
    ext.l   %d0                 | sign extend
    move.b  %d6, %d3            | copy b1
    lsr.l   #8, %d6             | prepare 1st buffer for next byte
    add.l   %d3, %d0            | add b1
    cmp.l   %d2, %d0            | overflow ?
    bls.b   1f
    spl.b   %d0                 |   yes: set appropriate limit value in low byte
1:
    move.w  (%a0), %d1          | load block[0]
    ext.l   %d1                 | sign extend
    move.b  %d6, %d3            | copy b0
    lsr.l   #8, %d6             | prepare 1st buffer for next byte
    add.l   %d3, %d1            | add b0
    cmp.l   %d2, %d1            | overflow ?
    bls.b   1f
    spl.b   %d1                 |   yes: set appropriate limit value in low byte
1:
    move.b  %d1, %d5            | collect output bytes 0..3 in %d5
    lsl.l   #8, %d5
    move.b  %d0, %d5
    lsl.l   #8, %d5
    clr.l   (%a0)+              | clear block[0] and block[1]
                                |   %a0 now pointing to block[2]
    move.w  (2,%a0), %d0        | do b3 and b2
    ext.l   %d0
    move.b  %d6, %d3
    lsr.l   #8, %d6
    add.l   %d3, %d0
    cmp.l   %d2, %d0
    bls.b   1f
    spl.b   %d0
1:
    move.w  (%a0), %d1
    ext.l   %d1
    add.l   %d6, %d1
    cmp.l   %d2, %d1
    bls.b   1f
    spl.b   %d1
1:
    move.b  %d1, %d5
    lsl.l   #8, %d5
    move.b  %d0, %d5
    clr.l   (%a0)+

    move.w  (2,%a0), %d0        | do b5 and b4
    ext.l   %d0
    move.b  %d7, %d3
    lsr.l   #8, %d7
    add.l   %d3, %d0
    cmp.l   %d2, %d0
    bls.b   1f
    spl.b   %d0
1:
    move.w  (%a0), %d1
    ext.l   %d1
    move.b  %d7, %d3
    lsr.l   #8, %d7
    add.l   %d3, %d1
    cmp.l   %d2, %d1
    bls.b   1f
    spl.b   %d1
1:
    move.b  %d1, %d6
    lsl.l   #8, %d6
    move.b  %d0, %d6
    lsl.l   #8, %d6
    clr.l   (%a0)+

    move.w  (2,%a0), %d0        | do b7 and b6
    ext.l   %d0
    move.b  %d7, %d3
    lsr.l   #8, %d7
    add.l   %d3, %d0
    cmp.l   %d2, %d0
    bls.b   1f
    spl.b   %d0
1:
    move.w  (%a0), %d1
    ext.l   %d1
    add.l   %d7, %d1
    cmp.l   %d2, %d1
    bls.b   1f
    spl.b   %d1
1:
    move.b  %d1, %d6
    lsl.l   #8, %d6
    move.b  %d0, %d6
    clr.l   (%a0)+

    movem.l %d5-%d6, (%a1)      | write all 8 output bytes at once
    lea.l   (%a2,%a1), %a1      | advance output pointer
    subq.l  #1, %d4             | loop 8 times
    bne.w   .add_clip_loop

    bra.w   .idct_add_end
    
.dc_add:
    move.w  (%a0), %d0
    ext.l   %d0                 | %d0 = (block[0]
    add.l   #64, %d0            |       + 64)
    asr.l   #7, %d0             |       >> 7
    clr.w   (%a0)               | clear block[0]
    clr.w   (63*2,%a0)          |   and block[63]
    move.l  %d0, %a0            | DC value in %a0
    
    move.l  #255, %d2           | preload constant for clipping
    clr.l   %d3                 | for splitting input words into bytes
    moveq.l #8, %d4             | loop counter
    
.dc_clip_loop:
    movem.l (%a1), %d6-%d7      | (b0 b1 b2 b3) (b4 b5 b6 b7)
    swap    %d6                 | (b2 b3 b0 b1)
    swap    %d7                 | (b6 b7 b4 b5)
    
    move.l  %a0, %d0            | copy DC
    move.b  %d6, %d3            | copy b1
    lsr.l   #8, %d6             | prepare 1st buffer for next byte
    add.l   %d3, %d0            | add b1
    cmp.l   %d2, %d0            | overflow ?
    bls.b   1f
    spl.b   %d0                 |   yes: set appropriate limit value in low byte
1:
    move.l  %a0, %d1            | copy DC
    move.b  %d6, %d3            | copy b0
    lsr.l   #8, %d6             | prepare 1st buffer for next byte
    add.l   %d3, %d1            | add b0
    cmp.l   %d2, %d1            | overflow ?
    bls.b   1f
    spl.b   %d1                 |   yes: set appropriate limit value in low byte
1:
    move.b  %d1, %d5            | collect output bytes 0..3 in %d5
    lsl.l   #8, %d5
    move.b  %d0, %d5
    lsl.l   #8, %d5

    move.l  %a0, %d0            | do b3 and b2
    move.b  %d6, %d3
    lsr.l   #8, %d6
    add.l   %d3, %d0
    cmp.l   %d2, %d0
    bls.b   1f
    spl.b   %d0
1:
    move.l  %a0, %d1
    add.l   %d6, %d1
    cmp.l   %d2, %d1
    bls.b   1f
    spl.b   %d1
1:
    move.b  %d1, %d5
    lsl.l   #8, %d5
    move.b  %d0, %d5

    move.l  %a0, %d0            | do b5 and b4
    move.b  %d7, %d3
    lsr.l   #8, %d7
    add.l   %d3, %d0
    cmp.l   %d2, %d0
    bls.b   1f
    spl.b   %d0
1:
    move.l  %a0, %d1
    move.b  %d7, %d3
    lsr.l   #8, %d7
    add.l   %d3, %d1
    cmp.l   %d2, %d1
    bls.b   1f
    spl.b   %d1
1:
    move.b  %d1, %d6            | do b7 and b6
    lsl.l   #8, %d6
    move.b  %d0, %d6
    lsl.l   #8, %d6

    move.l  %a0, %d0
    move.b  %d7, %d3
    lsr.l   #8, %d7
    add.l   %d3, %d0
    cmp.l   %d2, %d0
    bls.b   1f
    spl.b   %d0
1:
    move.l  %a0, %d1
    add.l   %d7, %d1
    cmp.l   %d2, %d1
    bls.b   1f
    spl.b   %d1
1:
    move.b  %d1, %d6
    lsl.l   #8, %d6
    move.b  %d0, %d6

    movem.l %d5-%d6, (%a1)      | write all 8 output bytes at once
    lea.l   (%a2,%a1), %a1      | advance output pointer
    subq.l  #1, %d4             | loop 8 times
    bne.w   .dc_clip_loop

.idct_add_end:
    movem.l (%sp), %d2-%d7/%a2-%a6
    lea.l   (11*4,%sp), %sp
    rts
