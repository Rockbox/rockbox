/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: lcd-as-video.S 26756 2010-06-11 04:41:36Z funman $
 *
 * Copyright (C) 2010 by Andree Buschmann
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

/**************************************************************************** 
 * #define FORCE_FIFO_WAIT
 *
 * This is not needed in YUV blitting when the LCD IF is fast enough. In this
 * case YUV-to-RGB conversion per pixel needs longer than the transfer of a 
 * pixel via the LCD IF. For iPod nano 2G this is true if the LCD IF is 
 * configured to use LCD_PHTIME = 0x00 (see lcd-nano2g.c).
 ****************************************************************************/

#include "config.h"

    .section .icode, "ax", %progbits
    
/****************************************************************************
 * void lcd_write_line(const fb_data *addr, 
 *                     int pixelcount, 
 *                     const unsigned int lcd_base_addr);
 * 
 * Writes pixelcount pixels from src-pointer (lcd_framebuffer) to LCD dataport.
 */
    .align  2
    .global lcd_write_line
    .type   lcd_write_line, %function
                                      /* r0 = addr, must be aligned */
                                      /* r1 = pixel count, must be even */
lcd_write_line:                       /* r2 = LCD_BASE */
    stmfd   sp!, {r4-r6, lr}          /* save non-scratch registers */
    add     r12, r2, #0x40            /* LCD_WDATA = LCD data port */

.loop:
    ldmia r0!, {r3, r5}               /* read 4 pixels (=8 byte) */

    /* wait for FIFO half full */
.fifo_wait:
    ldr     lr, [r2, #0x1C]           /* while (LCD_STATUS & 0x08); */
    tst     lr, #0x8
    bgt     .fifo_wait
    
    mov     r4, r3, asr #16           /* r3 = 1st pixel, r4 = 2nd pixel */
    mov     r6, r5, asr #16           /* r5 = 3rd pixel, r6 = 4th pixel */
    stmia   r12, {r3-r6}              /* write pixels (lowest 16 bit used) */

    subs    r1, r1, #4
    bgt     .loop

    ldmpc   regs=r4-r6
