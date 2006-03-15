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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "pacbox.h"

	.section .text
	.align  2
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

blit_display:
	lea	-28(%sp), %sp
	movem.l	%d2-%d6/%a2, (%sp)

	move.l	32(%sp), %a0	// lcd_framebuffer
	move.l	36(%sp), %a2	// vbuf

	lea	palette, %a1

	lea.l	((YOFS * LCD_WIDTH + XOFS + SHRUNK_HEIGHT) * 2)(%a0), %a0

	move.l	#(ScreenHeight / 4), %d0
	move.l	#(ScreenWidth / 4), %d1
	moveq.l	#0, %d2

y_loop:
	move.l	%d1, %d5
x_loop1:
	move.l	(%a2)+, %d3

	move.b	%d3, %d2
	move.w	(%a1, %d2.l * 2), %d6
	move.w	%d6, (2 * LCD_WIDTH * 2)(%a0)

	lsr.l	#8, %d3
	move.b	%d3, %d2
	move.w	(%a1, %d2.l * 2), %d6
	move.w	%d6, (1 * LCD_WIDTH * 2)(%a0)

	swap.w	%d3
	move.b	%d3, %d2
	move.w	(%a1, %d2.l * 2), (%a0)

	lea.l	3 * LCD_WIDTH * 2(%a0), %a0

	subq.l	#1, %d5
	bne.s	x_loop1

	suba.l	#(LCD_WIDTH * SHRUNK_WIDTH + 1) * 2, %a0
	lea.l	ScreenWidth(%a2), %a2	// Skip 1 line

	move.l	%d1, %d5
x_loop2:
	move.l	(%a2)+, %d3

	move.b	%d3, %d2
	move.w	(%a1, %d2.l * 2), %d6
	move.w	%d6, 2 * LCD_WIDTH * 2(%a0)

	lsr.l	#8, %d3
	move.b	%d3, %d2
	move.w	(%a1, %d2.l * 2), %d6
	move.w	%d6, 1 * LCD_WIDTH * 2(%a0)

	swap.w	%d3
	move.b	%d3, %d2
	move.w	(%a1, %d2.l * 2), (%a0)

	lea.l	3 * LCD_WIDTH * 2(%a0), %a0

	subq.l	#1, %d5
	bne.s	x_loop2

	suba.l	#(LCD_WIDTH * SHRUNK_WIDTH + 1) * 2, %a0

	move.l	%d1, %d5
x_loop3:
	move.l	(%a2)+, %d3

	move.b	%d3, %d2
	move.w	(%a1, %d2.l * 2), %d6
	move.w	%d6, 2 * LCD_WIDTH * 2(%a0)

	lsr.l	#8, %d3
	move.b	%d3, %d2
	move.w	(%a1, %d2.l * 2), %d6
	move.w	%d6, 1 * LCD_WIDTH * 2(%a0)

	swap.w	%d3
	move.b	%d3, %d2
	move.w	(%a1, %d2.l * 2), (%a0)

	lea.l	3 * LCD_WIDTH * 2(%a0), %a0

	subq.l	#1, %d5
	bne.s	x_loop3

	suba.l	#(LCD_WIDTH * SHRUNK_WIDTH + 1) * 2, %a0

	subq.l	#1, %d0
	bne	y_loop

	movem.l	(%sp), %d2-%d6/%a2
	lea.l	28(%sp), %sp
	rts

