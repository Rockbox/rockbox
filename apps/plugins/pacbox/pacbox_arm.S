/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2006 Dave Chapman
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

    .section .icode,"ax",%progbits
    .global blit_display

/* void blit_display(fb_data* lcd_framebuffer, unsigned char* vbuf)

   next_dst=&lcd_framebuffer[YOFS*LCD_WIDTH+XOFS+ScreenHeight-1];
   for( y=ScreenHeight; y>0; y-- ) {
       dst = (next_dst--);
       for( x=ScreenWidth; x>0; x-- ) {
           *dst = palette[*(vbuf++)];
           dst+=LCD_WIDTH;
       }
   }
 */

#ifdef HAVE_LCD_COLOR
#if (LCD_WIDTH >= 288) && (LCD_HEIGHT >= 224)

blit_display:
         stmdb   sp!, {r4-r11, lr}

         add     r3, r0, #5696            
         add     r3, r3, #24  @ 5720 = (2*(YOFS*LCD_WIDTH+XOFS+ScreenHeight-4))
         ldr     r0, =palette
         mov     lr, #288                 @ y = 288

loop_y:  mov     r2, r3                   @ r2 = next_dst
         sub     r3, r3, #8               @ next_dst-=4

         mov     ip, #224                 @ x = 224

/* store 2 input bytes from the next four lines in r7-r10 */
loop_x:  
         ldrh    r8,  [r1, #224]          @ r8 = vbuf[224]
         ldrh    r7,  [r1]                @ r7 = vbuf[0]
         add     r1,  r1, #448
         ldrh    r10,  [r1, #224]         @ r10 = vbuf[224*3]
         ldrh    r9,  [r1], #2            @ r9 = vbuf[224*2] ; vbuf += 2;
         sub     r1,  r1, #448


/* Convert high bytes of r7-r10 into palette entries in r5 and r6 */
         mov     r6, r7, lsr #8
         mov     r6, r6, lsl #1
         ldrh    r6, [r6, r0]             @ r6 = palette[hi(r7]]

         mov     r11, r8, lsr #8
         mov     r11, r11, lsl #1
         ldrh    r11, [r11, r0]           @ r11 = palette[hi(r8]]

         orr     r6, r11, r6, lsl #16     @ r6 = palette[hi(r8]]
                                          @ | (palette[hi(r7)] << 16)

         mov     r5, r9, lsr #8
         mov     r5, r5, lsl #1
         ldrh    r5, [r5, r0]             @ r5 = palette[hi(r9]]

         mov     r11, r10, lsr #8
         mov     r11, r11, lsl #1
         ldrh    r11, [r11, r0]           @ r11 = palette[hi(r10)]]

         orr     r5, r11, r5, lsl #16     @ r5 = palette[hi(r10]]
                                          @ | (palette[hi(r9)] << 16)

/* Convert low bytes of r7-r10 into palette entries in r7 and r8 */
         and     r7, r7, #0xff
         mov     r7, r7, lsl #1
         ldrh    r7, [r7, r0]

         and     r8, r8, #0xff
         mov     r8, r8, lsl #1
         ldrh    r8, [r8, r0]

         orr     r8, r8, r7, lsl #16

         and     r9, r9, #0xff
         mov     r9, r9, lsl #1
         ldrh    r9, [r9, r0]

         and     r10, r10, #0xff
         mov     r10, r10, lsl #1
         ldrh    r10, [r10, r0]

         orr     r7, r10, r9, lsl #16

/* Now write the 8 pixels to the screen */
         stmia   r2!, {r7, r8}
         add     r2, r2, #(LCD_WIDTH*2)-8 @ dst += LCD_WIDTH

         stmia   r2!, {r5, r6}
         add     r2, r2, #(LCD_WIDTH*2)-8 @ dst += LCD_WIDTH

/* end of x loop */
         subs    ip, ip, #2               @ x-=2
         bne     loop_x

/* end of y loop */
         add     r1, r1, #224*3           @ vbuf += 224*3
         subs    lr, lr, #4               @ y-=4
         ldmeqia sp!, {r4-r11, pc}
         b       loop_y
#endif
#endif
