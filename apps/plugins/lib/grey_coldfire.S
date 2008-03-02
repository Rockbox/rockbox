/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id: grey_draw.c 16080 2008-01-13 18:39:09Z amiconn $
*
* New greyscale framework
* Coldfire assembler routines
*
* This is a generic framework to display 129 shades of grey on low-depth
* bitmap LCDs (Archos b&w, Iriver & Ipod 4-grey) within plugins.
*
* Copyright (C) 2008 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "config.h"
/* Plugins should not normally do this, but we need to check a macro, and
 * plugin.h would confuse the assembler. */
 
    .text
    .global     _grey_line1
    .type       _grey_line1, @function
    
#if (LCD_PIXELFORMAT == VERTICAL_PACKING) && (LCD_DEPTH == 2)
    
/****************************************************************************
 * void _grey_line1(int width,
 *                  unsigned char *dst,
 *                  const unsigned char *src,
 *                  const unsigned char *lut);
 */

_grey_line1:
    lea.l   (-2*4, %sp), %sp
    movem.l %d2/%a2, (%sp)
    movem.l (2*4+4, %sp), %d2/%a0-%a2
    clr.l   %d0

    move.l  %a1, %d1
    and.l   #1, %d1
    beq.s   .p1_h_end

    move.b  (%a1)+, %d0
    move.b  (%d0.l, %a2), (%a0)
    addq.l  #4, %a0
    subq.l  #1, %d2
.p1_h_end:

    cmp.l   #2, %d2
    blo.s   .p2_t_end
    move.l  %a1, %d1
    and.l   #2, %d1
    beq.s   .p2_h_end
    
    move.w  (%a1)+, %d1
    move.w  %d1, %d0
    lsr.l   #8, %d0
    move.b  (%d0.l, %a2), (%a0)
    addq.l  #4, %a0
    move.b  %d1, %d0
    move.b  (%d0.l, %a2), (%a0)
    addq.l  #4, %a0
    subq.l  #2, %d2
.p2_h_end:

    subq.l  #4, %d2
    blo.s   .p4_end
    
.p4_loop:
    move.l  (%a1)+, %d1
    swap    %d1
    move.w  %d1, %d0
    lsr.l   #8, %d0
    move.b  (%d0.l, %a2), (%a0)
    addq.l  #4, %a0
    move.b  %d1, %d0
    move.b  (%d0.l, %a2), (%a0)
    addq.l  #4, %a0
    swap    %d1
    move.w  %d1, %d0
    lsr.l   #8, %d0
    move.b  (%d0.l, %a2), (%a0)
    addq.l  #4, %a0
    move.b  %d1, %d0
    move.b  (%d0.l, %a2), (%a0)
    addq.l  #4, %a0
    subq.l  #4, %d2
    bhs.s   .p4_loop
    
.p4_end:
    addq.l  #4, %d2
    cmp.l   #2, %d2
    blo.s   .p2_t_end
    
    move.w  (%a1)+, %d1
    move.w  %d1, %d0
    lsr.l   #8, %d0
    move.b  (%d0.l, %a2), (%a0)
    addq.l  #4, %a0
    move.b  %d1, %d0
    move.b  (%d0.l, %a2), (%a0)
    addq.l  #4, %a0
    subq.l  #2, %d2
.p2_t_end:

    tst.l   %d2
    beq.s   .p1_t_end

    move.b  (%a1)+, %d0
    move.b  (%d0.l, %a2), (%a0)
.p1_t_end:

    movem.l (%sp), %d2/%a2
    lea.l   (2*4, %sp), %sp
    rts
    .size   _grey_line1, . - _grey_line1

#endif
