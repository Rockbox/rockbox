/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* New greyscale framework
* Coldfire assembler routines
*
* This is a generic framework to display 129 shades of grey on low-depth
* bitmap LCDs (Archos b&w, Iriver & Ipod 4-grey) within plugins.
*
* Copyright (C) 2008 Jens Arnold
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
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
    
#if (LCD_PIXELFORMAT == VERTICAL_PACKING) \
 || (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)
 
#if (LCD_DEPTH == 1) || (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)
#define GREY_BSIZE 8
#elif LCD_DEPTH == 2
#define GREY_BSIZE 4
#endif
    
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
    addq.l  #GREY_BSIZE, %a0
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
    addq.l  #GREY_BSIZE, %a0
    move.b  %d1, %d0
    move.b  (%d0.l, %a2), (%a0)
    addq.l  #GREY_BSIZE, %a0
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
    addq.l  #GREY_BSIZE, %a0
    move.b  %d1, %d0
    move.b  (%d0.l, %a2), (%a0)
    addq.l  #GREY_BSIZE, %a0
    swap    %d1
    move.w  %d1, %d0
    lsr.l   #8, %d0
    move.b  (%d0.l, %a2), (%a0)
    addq.l  #GREY_BSIZE, %a0
    move.b  %d1, %d0
    move.b  (%d0.l, %a2), (%a0)
    addq.l  #GREY_BSIZE, %a0
    subq.l  #4, %d2
    bhs.s   .p4_loop
    
    /* No need to correct the count, we're only testing bits from now on. */

.p4_end:
    btst.l  #1, %d2
    beq.s   .p2_t_end
    
    move.w  (%a1)+, %d1
    move.w  %d1, %d0
    lsr.l   #8, %d0
    move.b  (%d0.l, %a2), (%a0)
    addq.l  #GREY_BSIZE, %a0
    move.b  %d1, %d0
    move.b  (%d0.l, %a2), (%a0)
    addq.l  #GREY_BSIZE, %a0
.p2_t_end:

    btst.l  #0, %d2
    beq.s   .p1_t_end

    move.b  (%a1)+, %d0
    move.b  (%d0.l, %a2), (%a0)
.p1_t_end:

    movem.l (%sp), %d2/%a2
    lea.l   (2*4, %sp), %sp
    rts
    .size   _grey_line1, . - _grey_line1

#endif
