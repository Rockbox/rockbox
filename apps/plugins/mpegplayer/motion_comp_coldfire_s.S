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
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
.macro LEFT8_PW   dW1, dW2      | needs %d0 == 24, clobbers %d2
    lsl.l   #8, \dW1            | changes dW1, keeps dW2
    move.l  \dW2, %d2
    lsr.l   %d0, %d2
    or.l    %d2, \dW1
.endm

.macro LEFT24_PW   dW1, dW2     | needs %d0 == 24, clobbers %d2
    lsl.l   %d0, \dW1           | changes dW1, keeps dW2
    move.l  \dW2, %d2
    lsr.l   #8, %d2
    or.l    %d2, \dW1
.endm

/*****************************************************************************/

    .align  2
    .global MC_put_o_8
    .type   MC_put_o_8, @function
    
MC_put_o_8:
    movem.l (4,%sp), %a0-%a1    | dest, source
    move.l  %a1, %d0
    and.l   #3, %d0
    sub.l   %d0, %a1            | align source
    jmp.l   (2, %pc, %d0.l*4)
    bra.w   .po8_0
    bra.w   .po8_1
    bra.w   .po8_2
    | last table entry coincides with target

.po8_3:
    lea.l   (-5*4,%sp), %sp
    movem.l %d2-%d5/%a2, (%sp)  | save some registers
    move.l  (5*4+12,%sp), %a2   | stride
    move.l  (5*4+16,%sp), %d1   | height
    moveq.l #24, %d0            | shift amount
1:
    movem.l (%a1), %d3-%d5
    add.l   %a2, %a1
    LEFT24_PW %d3, %d4
    lsl.l   %d0, %d4
    lsr.l   #8, %d5
    or.l    %d5, %d4
    movem.l %d3-%d4, (%a0)
    add.l   %a2, %a0
    subq.l  #1, %d1
    bne.s   1b
    movem.l (%sp), %d2-%d5/%a2
    lea.l   (5*4,%sp), %sp
    rts

.po8_2:
    lea.l   (-3*4,%sp), %sp
    movem.l %d2-%d4, (%sp)          | save some registers
    movem.l (3*4+12,%sp), %d0-%d1   | stride, height
1:
    movem.l (%a1), %d2-%d4
    add.l   %d0, %a1
    swap    %d2
    swap    %d3
    move.w  %d3, %d2
    swap    %d4
    move.w  %d4, %d3
    movem.l %d2-%d3, (%a0)
    add.l   %d0, %a0
    subq.l  #1, %d1
    bne.s   1b
    movem.l (%sp), %d2-%d4
    lea.l   (3*4,%sp), %sp
    rts

.po8_1:
    lea.l   (-5*4,%sp), %sp
    movem.l %d2-%d5/%a2, (%sp)  | save some registers
    move.l  (5*4+12,%sp), %a2   | stride
    move.l  (5*4+16,%sp), %d1   | height
    moveq.l #24, %d0            | shift amount
1:
    movem.l (%a1), %d3-%d5
    add.l   %a2, %a1
    LEFT8_PW %d3, %d4
    lsl.l   #8, %d4
    lsr.l   %d0, %d5
    or.l    %d5, %d4
    movem.l %d3-%d4, (%a0)
    add.l   %a2, %a0
    subq.l  #1, %d1
    bne.s   1b
    movem.l (%sp), %d2-%d5/%a2
    lea.l   (5*4,%sp), %sp
    rts

.po8_0:
    movem.l (12,%sp), %d0-%d1   | stride, height
    subq.l  #4, %d0             | adjust for increment within the loop
1:
    move.l  (%a1)+, (%a0)+
    move.l  (%a1), (%a0)
    add.l   %d0, %a0
    add.l   %d0, %a1
    subq.l  #1, %d1
    bne.s   1b
    rts
    
/*****************************************************************************/

    .align  2
    .global MC_put_o_16
    .type   MC_put_o_16, @function

MC_put_o_16:
    lea.l   (-7*4,%sp), %sp
    movem.l %d2-%d7/%a2, (%sp)  | save some registers
    movem.l (7*4+4,%sp), %a0-%a2| dest, source, stride
    move.l  (7*4+16,%sp), %d1   | height
    move.l  %a1, %d0
    and.l   #3, %d0
    sub.l   %d0, %a1
    jmp.l   (2, %pc, %d0.l*4)
    bra.w   .po16_0
    bra.w   .po16_1
    bra.w   .po16_2
    | last table entry coincides with target

.po16_3:
    moveq.l #24, %d0            | shift amount
1:
    movem.l (%a1), %d3-%d7
    add.l   %a2, %a1
    LEFT24_PW %d3, %d4
    LEFT24_PW %d4, %d5
    LEFT24_PW %d5, %d6
    lsl.l   %d0, %d6
    lsr.l   #8, %d7
    or.l    %d7, %d6
    movem.l %d3-%d6, (%a0)
    add.l   %a2, %a0
    subq.l  #1, %d1
    bne.s   1b
    movem.l (%sp), %d2-%d7/%a2
    lea.l   (7*4,%sp), %sp
    rts

.po16_2:
1:
    movem.l (%a1), %d3-%d7
    add.l   %a2, %a1
    swap    %d3
    swap    %d4
    move.w  %d4, %d3
    swap    %d5
    move.w  %d5, %d4
    swap    %d6
    move.w  %d6, %d5
    swap    %d7
    move.w  %d7, %d6
    movem.l %d3-%d6, (%a0)
    add.l   %a2, %a0
    subq.l  #1, %d1
    bne.s   1b
    movem.l (%sp), %d2-%d7/%a2
    lea.l   (7*4,%sp), %sp
    rts

.po16_1:
    moveq.l #24, %d0            | shift amount
1:
    movem.l (%a1), %d3-%d7
    add.l   %a2, %a1
    LEFT8_PW %d3, %d4
    LEFT8_PW %d4, %d5
    LEFT8_PW %d5, %d6
    lsl.l   #8, %d6
    lsr.l   %d0, %d7
    or.l    %d7, %d6
    movem.l %d3-%d6, (%a0)
    add.l   %a2, %a0
    subq.l  #1, %d1
    bne.s   1b
    movem.l (%sp), %d2-%d7/%a2
    lea.l   (7*4,%sp), %sp
    rts

.po16_0:
1:
    movem.l (%a1), %d3-%d6
    add.l   %a2, %a1
    movem.l %d3-%d6, (%a0)
    add.l   %a2, %a0
    subq.l  #1, %d1
    bne.s   1b
    movem.l (%sp), %d2-%d7/%a2
    lea.l   (7*4,%sp), %sp
    rts
    
/*****************************************************************************/

.macro  AVG_PW  dW1, dW2        | needs %d0 == 24, clobbers %d1, %d2,
    move.l  \dW1, %d1           | changes dW1, keeps dW2
    lsl.l   #8, \dW1
    move.l  \dW2, %d2
    lsr.l   %d0, %d2
    or.l    %d2, \dW1
    move.l  %d1, %d2
    eor.l   \dW1, %d1
    and.l   %d2, \dW1
    move.l  #0xfefefefe, %d2
    and.l   %d1, %d2
    eor.l   %d2, %d1
    lsr.l   #1, %d2
    add.l   %d2, \dW1
    add.l   %d1, \dW1
.endm

/*****************************************************************************/

    .align  2
    .global MC_put_x_8
    .type   MC_put_x_8, @function
    
MC_put_x_8:
    lea.l   (-6*4,%sp), %sp
    movem.l %d2-%d6/%a2, (%sp)  | save some registers
    movem.l (6*4+4,%sp), %a0-%a2| dest, source, stride
    move.l  (6*4+16,%sp), %d6   | height
    move.l  %a1, %d0
    and.l   #3, %d0
    sub.l   %d0, %a1
    jmp.l   (2, %pc, %d0.l*4)
    bra.w   .px8_0
    bra.w   .px8_1
    bra.w   .px8_2
    | last table entry coincides with target

.px8_3:
    moveq.l #24, %d0
1:
    movem.l (%a1), %d3-%d5
    add.l   %a2, %a1
    LEFT24_PW %d3, %d4
    LEFT24_PW %d4, %d5
    lsl.l   %d0, %d5
    AVG_PW  %d3, %d4
    AVG_PW  %d4, %d5
    movem.l %d3-%d4, (%a0)
    add.l   %a2, %a0
    subq.l  #1, %d6
    bne.s   1b
    movem.l (%sp), %d2-%d6/%a2
    lea.l   (6*4,%sp), %sp
    rts

.px8_2:
    moveq.l #24, %d0
1:
    movem.l (%a1), %d3-%d5
    add.l   %a2, %a1
    swap    %d3
    swap    %d4
    move.w  %d4, %d3
    swap    %d5
    move.w  %d5, %d4
    AVG_PW  %d3, %d4
    AVG_PW  %d4, %d5
    movem.l %d3-%d4, (%a0)
    add.l   %a2, %a0
    subq.l  #1, %d6
    bne.s   1b
    movem.l (%sp), %d2-%d6/%a2
    lea.l   (6*4,%sp), %sp
    rts

.px8_1:
    moveq.l #24, %d0
1:
    movem.l (%a1), %d3-%d5
    add.l   %a2, %a1
    LEFT8_PW %d3, %d4
    LEFT8_PW %d4, %d5
    lsl.l   #8, %d5
    AVG_PW  %d3, %d4
    AVG_PW  %d4, %d5
    movem.l %d3-%d4, (%a0)
    add.l   %a2, %a0
    subq.l  #1, %d6
    bne.s   1b
    movem.l (%sp), %d2-%d6/%a2
    lea.l   (6*4,%sp), %sp
    rts

.px8_0:
    moveq.l #24, %d0
1:
    movem.l (%a1), %d3-%d5
    add.l   %a2, %a1
    AVG_PW  %d3, %d4
    AVG_PW  %d4, %d5
    movem.l %d3-%d4, (%a0)
    add.l   %a2, %a0
    subq.l  #1, %d6
    bne.s   1b
    movem.l (%sp), %d2-%d6/%a2
    lea.l   (6*4,%sp), %sp
    rts
    
/*****************************************************************************/

    .align  2
    .global MC_put_x_16
    .type   MC_put_x_16, @function
    
MC_put_x_16:
    lea.l   (-8*4,%sp), %sp
    movem.l %d2-%d7/%a2-%a3, (%sp)  | save some registers
    movem.l (8*4+4,%sp), %a0-%a3    | dest, source, stride, height
    move.l  %a1, %d0
    and.l   #3, %d0
    sub.l   %d0, %a1
    jmp.l   (2, %pc, %d0.l*4)
    bra.w   .px16_0
    bra.w   .px16_1
    bra.w   .px16_2
    | last table entry coincides with target

.px16_3:
    moveq.l #24, %d0
1:
    movem.l (%a1), %d3-%d7
    add.l   %a2, %a1
    LEFT24_PW %d3, %d4
    LEFT24_PW %d4, %d5
    LEFT24_PW %d5, %d6
    LEFT24_PW %d6, %d7
    lsl.l   %d0, %d7
    AVG_PW  %d3, %d4
    AVG_PW  %d4, %d5
    AVG_PW  %d5, %d6
    AVG_PW  %d6, %d7
    movem.l %d3-%d6, (%a0)
    add.l   %a2, %a0
    subq.l  #1, %a3
    tst.l   %a3
    bne.w   1b
    movem.l (%sp), %d2-%d7/%a2-%a3
    lea.l   (8*4,%sp), %sp
    rts

.px16_2:
    moveq.l #24, %d0
1:
    movem.l (%a1), %d3-%d7
    add.l   %a2, %a1
    swap    %d3
    swap    %d4
    move.w  %d4, %d3
    swap    %d5
    move.w  %d5, %d4
    swap    %d6
    move.w  %d6, %d5
    swap    %d7
    move.w  %d7, %d6
    AVG_PW  %d3, %d4
    AVG_PW  %d4, %d5
    AVG_PW  %d5, %d6
    AVG_PW  %d6, %d7
    movem.l %d3-%d6, (%a0)
    add.l   %a2, %a0
    subq.l  #1, %a3
    tst.l   %a3
    bne.w   1b
    movem.l (%sp), %d2-%d7/%a2-%a3
    lea.l   (8*4,%sp), %sp
    rts

.px16_1:
    moveq.l #24, %d0
1:
    movem.l (%a1), %d3-%d7
    add.l   %a2, %a1
    LEFT8_PW %d3, %d4
    LEFT8_PW %d4, %d5
    LEFT8_PW %d5, %d6
    LEFT8_PW %d6, %d7
    lsl.l   #8, %d7
    AVG_PW  %d3, %d4
    AVG_PW  %d4, %d5
    AVG_PW  %d5, %d6
    AVG_PW  %d6, %d7
    movem.l %d3-%d6, (%a0)
    add.l   %a2, %a0
    subq.l  #1, %a3
    tst.l   %a3
    bne.w   1b
    movem.l (%sp), %d2-%d7/%a2-%a3
    lea.l   (8*4,%sp), %sp
    rts

.px16_0:
    moveq.l #24, %d0
1:
    movem.l (%a1), %d3-%d7
    add.l   %a2, %a1
    AVG_PW  %d3, %d4
    AVG_PW  %d4, %d5
    AVG_PW  %d5, %d6
    AVG_PW  %d6, %d7
    movem.l %d3-%d6, (%a0)
    add.l   %a2, %a0
    subq.l  #1, %a3
    tst.l   %a3
    bne.w   1b
    movem.l (%sp), %d2-%d7/%a2-%a3
    lea.l   (8*4,%sp), %sp
    rts
