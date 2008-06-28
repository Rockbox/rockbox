/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2006 Fredrik Ohrn
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

#include "pacbox.h"

	.section .text
	.align  2


#if defined(IRIVER_H300_SERIES) && !defined(SIMULATOR)

	.global blit_display

/*

 0.75 scaling - display 3 out of 4 pixels = 216x168
 Skipping pixel #2 out of 4 seems to give the most legible display
 
void blit_display(fb_data* lcd_framebuffer, unsigned char* vbuf)
{
    next_dst=&lcd_framebuffer[YOFS*LCD_WIDTH+XOFS+((ScreenHeight*3)/4)-1];
    for (y=ScreenHeight-1;y >= 0; y--) {
        if ((y & 3) != 1) {
            dst = (next_dst--);
            for (x=0;x<ScreenWidth;x++) {
                if ((x & 3) == 1) { vbuf++; }
                else {
                   *dst = palette[*(vbuf++)];
                   dst+=LCD_WIDTH;
                }
            }
        } else {
            vbuf+=ScreenWidth;
        }
    }
}
*/

/* FIXME: ScreenWidth & ScreenHeight are defined in arcade.h, but all
          hell breaks loose if you include it... */

#define ScreenWidth 224
#define ScreenHeight 288

#define SHRUNK_WIDTH ((ScreenWidth/4)*3)
#define SHRUNK_HEIGHT ((ScreenHeight/4)*3)

#define BD_SAVE (9 * 4)

process_4_pixels:
	move.b	%d4, %d2
	move.w	(%a1, %d2.l * 2), %d3
	move.w	%d3, (2 * LCD_WIDTH * 2)(%a0)

	lsr.l	#8, %d4
	move.b	%d4, %d2
	move.w	(%a1, %d2.l * 2), %d3
	move.w	%d3, (1 * LCD_WIDTH * 2)(%a0)

	swap.w	%d4
	move.b	%d4, %d2
	move.w	(%a1, %d2.l * 2), (%a0)

	lea.l	3 * LCD_WIDTH * 2(%a0), %a0
	rts

process_line:
	move.l	%a3, %d1
bd_x_loop:
	movem.l	(%a2), %d4-%d7	// Read 16 pixels

	bsr.s	process_4_pixels
	move.l	%d5, %d4
	bsr.s	process_4_pixels
	move.l	%d6, %d4
	bsr.s	process_4_pixels
	move.l	%d7, %d4
	bsr.s	process_4_pixels

	lea	16(%a2), %a2
	subq.l	#1, %d1;
	bne.s	bd_x_loop

	rts

blit_display:
	lea	-BD_SAVE(%sp), %sp
	movem.l	%d2-%d7/%a2-%a4, (%sp)

	move.l	BD_SAVE + 4(%sp), %a0	// lcd_framebuffer
	move.l	BD_SAVE + 8(%sp), %a2	// vbuf

	lea	palette, %a1

	lea.l	((YOFS * LCD_WIDTH + XOFS + SHRUNK_HEIGHT) * 2)(%a0), %a0

	move.l	#(ScreenHeight / 4), %d0
	move.l	#(ScreenWidth / 16), %a3
	move.l	#(LCD_WIDTH * SHRUNK_WIDTH + 1) * 2, %a4
	moveq.l	#0, %d2

bd_y_loop:
	bsr.s	process_line

	suba.l	%a4, %a0
	lea.l	ScreenWidth(%a2), %a2	// Skip 1 line

	bsr.s	process_line

	suba.l	%a4, %a0

	bsr.s	process_line

	suba.l	%a4, %a0

	subq.l	#1, %d0
	bne	bd_y_loop

	movem.l	(%sp), %d2-%d7/%a2-%a4
	lea.l	BD_SAVE(%sp), %sp
	rts

#endif /* defined(IRIVER_H300_SERIES) && !defined(SIMULATOR) */


/* See arcade.c for the C implementation of drawChar */
/* Note! This version does not handle flipped screen mode. */
 
	.global drawChar

#define DC_SAVE (4 * 4)

drawChar:
	lea	-DC_SAVE(%sp), %sp
	movem.l	%d2-%d5, (%sp)

	movea.l	DC_SAVE + 4(%sp), %a0	// buffer

	move.l	DC_SAVE + 16(%sp), %d0	// oy
	move.l	%d0, %d1
	lsl.l	#8, %d0
	lsl.l	#5, %d1
	sub.l	%d1, %d0
	adda.l	%d0, %a0
	adda.l	DC_SAVE + 12(%sp), %a0	// ox

	move.l	DC_SAVE + 20(%sp), %d0	// color
	and.l	#0x3F, %d0
	bne.s	have_color

	moveq.l	#8, %d1
	moveq.l	#0, %d2
	moveq.l	#0, %d3

clear_loop:
	movem.l	%d2-%d3, (%a0)
	lea	224(%a0), %a0

	subq.l	#1, %d1
	bne.s	clear_loop

	bra.s	dc_exit

have_color:
	lsl.l	#2, %d0

	lea.l	charmap_, %a1
	move.l	DC_SAVE + 8(%sp), %d2	// index
	lsl.l	#6, %d2
	adda.l	%d2, %a1

	moveq.l	#8, %d1
	moveq.l	#0, %d3
	moveq.l	#24, %d5

dc_y_loop:
	move.l	(%a1)+, %d3
	move.l	%d3, %d4

	move.l	%d3, %d2	// Pixel 1
	lsr.l	%d5, %d2
	add.l	%d0, %d2
	lsl.l	#8, %d2

	swap.w	%d4		// Pixel 2
	move.b	%d4, %d2
	add.l	%d0, %d2
	lsl.l	#8, %d2

	lsr.l	#8, %d3		// Pixel 3
	move.b	%d3, %d2
	add.l	%d0, %d2
	lsl.l	#8, %d2

	swap.w	%d4		// Pixel 4
	move.b	%d4, %d2
	add.l	%d0, %d2

	move.l	%d2, (%a0)+

	move.l	(%a1)+, %d3
	move.l	%d3, %d4

	move.l	%d3, %d2	// Pixel 1
	lsr.l	%d5, %d2
	add.l	%d0, %d2
	lsl.l	#8, %d2

	swap.w	%d4		// Pixel 2
	move.b	%d4, %d2
	add.l	%d0, %d2
	lsl.l	#8, %d2

	lsr.l	#8, %d3		// Pixel 3
	move.b	%d3, %d2
	add.l	%d0, %d2
	lsl.l	#8, %d2

	swap.w	%d4		// Pixel 4
	move.b	%d4, %d2
	add.l	%d0, %d2

	move.l	%d2, (%a0)+

	lea	216(%a0), %a0

	subq.l	#1, %d1
	bne.s	dc_y_loop

dc_exit:
	movem.l	(%sp), %d2-%d5
	lea.l	DC_SAVE(%sp), %sp
	rts

