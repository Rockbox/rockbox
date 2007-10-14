/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "cpu.h"

    .section    .icode, "ax", @progbits

/* lcd_write_yuv420_lines()
 *
 * See http://en.wikipedia.org/wiki/YCbCr
 *   ITU-R BT.601 (formerly CCIR 601):
 *   |Y'|   | 0.299000  0.587000  0.114000| |R|
 *   |Pb| = |-0.168736 -0.331264  0.500000| |G| or 0.564334*(B - Y')
 *   |Pr|   | 0.500000 -0.418688  0.081312| |B| or 0.713267*(R - Y')
 *   Scaled, normalized and rounded:
 *   |Y'|   | 65  129   25| |R| +  16 : 16->235
 *   |Cb| = |-38  -74  112| |G| + 128 : 16->240
 *   |Cr|   |112  -94  -18| |B| + 128 : 16->240
 *
 *   The inverse:
 *   |R|   |1.000000  0.000000  1.402000| |Y'|
 *   |G| = |1.000000 -0.334136 -0.714136| |Pb|
 *   |B|   |1.000000  1.772000  0.000000| |Pr|
 *   Scaled, normalized, rounded and tweaked to yield RGB565:
 *   |R|   |19611723        0  26881894| |Y' -  16| >> 27
 *   |G| = |19611723 -6406711 -13692816| |Cb - 128| >> 26
 *   |B|   |19611723 33976259         0| |Cr - 128| >> 27
 *
 * Needs EMAC set to saturated, signed integer mode.
 *
 * register usage:
 *   %a0 - LCD data port
 *   %a1 - Y pointer
 *   %a2 - C pointer
 *   %a3 - C width
 *   %a4 - Y end address
 *   %a5 - Y factor
 *   %a6 - BU factor
 *   %d0 - scratch
 *   %d1 -     B, previous Y \ alternating
 *   %d2 - U / B, previous Y /
 *   %d3 - V / G
 *   %d4 - R / output pixel
 *   %d5 - GU factor
 *   %d6 - GV factor
 *   %d7 - RGB signed -> unsigned conversion mask
 */
    .align      2
    .global     lcd_write_yuv420_lines
    .type       lcd_write_yuv420_lines, @function

lcd_write_yuv420_lines:
    lea.l   (-44, %sp), %sp         /* free up some registers */
    movem.l %d2-%d7/%a2-%a6, (%sp)
    
    lea.l   0xf0000002, %a0         /* LCD data port */
    movem.l (44+4, %sp), %a1-%a3    /* Y data, C data, C width */
    lea.l   (%a1, %a3*2), %a4       /* Y end address */

    move.l  #19611723, %a5          /* y factor */
    move.l  #33976259, %a6          /* bu factor */
    move.l  #-6406711, %d5          /* gu factor */
    move.l  #-13692816, %d6         /* gv factor */
    move.l  #0x8410, %d7            /* bitmask for signed->unsigned conversion
                                     * of R, G and B within RGB565 at once */

    /* chroma for first 2x2 pixel block */
    clr.l   %d3                     /* load v component */
    move.b  (%a2, %a3), %d3
    clr.l   %d2                     /* load u component */
    move.b  (%a2)+, %d2
    moveq.l #-128, %d0
    add.l   %d0, %d2
    add.l   %d0, %d3

    mac.l   %a6, %d2, %acc0         /* bu */
    mac.l   %d5, %d2, %acc1         /* gu */
    mac.l   %d6, %d3, %acc1         /* gv */
    move.l  #26881894, %d0          /* rv factor */
    mac.l   %d0, %d3, %acc2         /* rv */

    /* luma for very first pixel (top left) */
    clr.l   %d1
    move.b  (%a1, %a3*2), %d1
    moveq.l #-126, %d0
    add.l   %d1, %d0                /* y' (-0.5 ... +0.5) */
    mac.l   %a5, %d0, %acc0
    mac.l   %a5, %d0, %acc1
    mac.l   %a5, %d0, %acc2

    bra.b   .yuv_line_entry

.yuv_line_loop:
    /* chroma for 2x2 pixel block */
    clr.l   %d3                     /* load v component */
    move.b  (%a2, %a3), %d3
    clr.l   %d2                     /* load u component */
    move.b  (%a2)+, %d2
    moveq.l #-128, %d0
    add.l   %d0, %d2
    add.l   %d0, %d3

    mac.l   %a6, %d2, %acc0         /* bu */
    mac.l   %d5, %d2, %acc1         /* gu */
    mac.l   %d6, %d3, %acc1         /* gv */
    move.l  #26881894, %d0          /* rv factor */
    mac.l   %d0, %d3, %acc2         /* rv */

    /* luma for first pixel (top left) */
    clr.l   %d1
    move.b  (%a1, %a3*2), %d1
    moveq.l #-126, %d0
    add.l   %d1, %d0                /* y' (-0.5 ... +0.5) */
    mac.l   %a5, %d0, %acc0
    mac.l   %a5, %d0, %acc1
    mac.l   %a5, %d0, %acc2

    move.w  %d4, (%a0)
    /* LCD write is delayed one pixel to use it for filling the EMAC latency */

    /* convert to RGB565, pack and output */
.yuv_line_entry:
    moveq.l #27, %d0
    move.l  %acc0, %d2
    move.l  %acc1, %d3
    move.l  %acc2, %d4
    lsr.l   %d0, %d2
    lsr.l   %d0, %d4
    moveq.l #26, %d0
    lsr.l   %d0, %d3
    lsl.l   #6, %d4
    or.l    %d3, %d4
    lsl.l   #5, %d4
    or.l    %d2, %d4
    eor.l   %d7, %d4

    /* luma for second pixel (bottom left) as delta from the first */
    clr.l   %d2
    move.b  (%a1)+, %d2
    move.l  %d2, %d0
    sub.l   %d1, %d0
    mac.l   %a5, %d0, %acc0
    mac.l   %a5, %d0, %acc1
    mac.l   %a5, %d0, %acc2

    move.w  %d4, (%a0)
    /* LCD write is delayed one pixel to use it for filling the EMAC latency */

    /* convert to RGB565, pack and output */
    moveq.l #27, %d0
    move.l  %acc0, %d1
    move.l  %acc1, %d3
    move.l  %acc2, %d4
    lsr.l   %d0, %d1
    lsr.l   %d0, %d4
    moveq.l #26, %d0
    lsr.l   %d0, %d3
    lsl.l   #6, %d4
    or.l    %d3, %d4
    lsl.l   #5, %d4
    or.l    %d1, %d4
    eor.l   %d7, %d4

    /* luma for third pixel (top right) as delta from the second */
    clr.l   %d1
    move.b  (%a1, %a3*2), %d1
    move.l  %d1, %d0
    sub.l   %d2, %d0
    mac.l   %a5, %d0, %acc0
    mac.l   %a5, %d0, %acc1
    mac.l   %a5, %d0, %acc2

    move.w  %d4, (%a0)
    /* LCD write is delayed one pixel to use it for filling the EMAC latency */

    /* convert to RGB565, pack and output */
    moveq.l #27, %d0
    move.l  %acc0, %d2
    move.l  %acc1, %d3
    move.l  %acc2, %d4
    lsr.l   %d0, %d2
    lsr.l   %d0, %d4
    moveq.l #26, %d0
    lsr.l   %d0, %d3
    lsl.l   #6, %d4
    or.l    %d3, %d4
    lsl.l   #5, %d4
    or.l    %d2, %d4
    eor.l   %d7, %d4

    /* luma for fourth pixel (bottom right) as delta from the third */
    clr.l   %d2
    move.b  (%a1)+, %d2
    move.l  %d2, %d0
    sub.l   %d1, %d0
    mac.l   %a5, %d0, %acc0
    mac.l   %a5, %d0, %acc1
    mac.l   %a5, %d0, %acc2

    move.w  %d4, (%a0)
    /* LCD write is delayed one pixel to use it for filling the EMAC latency */

    /* convert to RGB565, pack and output */
    moveq.l #27, %d0
    movclr.l %acc0, %d1
    movclr.l %acc1, %d3
    movclr.l %acc2, %d4
    lsr.l   %d0, %d1
    lsr.l   %d0, %d4
    moveq.l #26, %d0
    lsr.l   %d0, %d3
    lsl.l   #6, %d4
    or.l    %d3, %d4
    lsl.l   #5, %d4
    or.l    %d1, %d4
    eor.l   %d7, %d4

    cmp.l   %a1, %a4                /* run %a1 up to end of line */
    bhi.w   .yuv_line_loop

    move.w  %d4, (%a0)              /* write (very) last pixel */

    movem.l (%sp), %d2-%d7/%a2-%a6
    lea.l   (44, %sp), %sp          /* restore registers */
    rts
.yuv_end:
    .size   lcd_write_yuv420_lines, .yuv_end - lcd_write_yuv420_lines
