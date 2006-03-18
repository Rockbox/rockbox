/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Pacbox - a Pacman Emulator for Rockbox
 *
 * Based on PIE - Pacman Instructional Emulator
 *
 * Copyright (c) 1997-2003,2004 Alessandro Scotti
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "pacbox.h"
#include "pacbox_lcd.h"
#include "arcade.h"
#include "hardware.h"

#if defined(SIMULATOR) || !defined(IRIVER_H300_SERIES)

void blit_display(fb_data* lcd_framebuffer, unsigned char* vbuf)
{
    fb_data* dst;
    fb_data* next_dst;
    int x,y;

#ifdef HAVE_LCD_COLOR
#if (LCD_WIDTH >= 224) && (LCD_HEIGHT >= 288)
        /* Native resolution = 224x288 */
        (void)next_dst;
        dst=&lcd_framebuffer[YOFS*LCD_WIDTH+XOFS];
        for (y=0;y<ScreenHeight;y++) {
            for (x=0;x<ScreenWidth;x++) {
                *(dst++) = palette[*(vbuf++)];
            }
            dst += XOFS*2;
        }
#elif (LCD_WIDTH >= 288) && (LCD_HEIGHT >= 224)
        /* Native resolution - rotated 90 degrees = 288x224 */
        next_dst=&lcd_framebuffer[YOFS*LCD_WIDTH+XOFS+ScreenHeight-1];
        for( y=ScreenHeight-1; y>=0; y-- ) {
            dst = (next_dst--);
            for( x=0; x<ScreenWidth; x++ ) {
                *dst = palette[*(vbuf++)];
                dst+=LCD_WIDTH;
            }
        }
#elif (LCD_WIDTH >= 216) && (LCD_HEIGHT >= 168)
        /* 0.75 scaling - display 3 out of 4 pixels = 216x168 
           Skipping pixel #2 out of 4 seems to give the most legible display 
         */
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
#elif (LCD_WIDTH >= 144) && (LCD_HEIGHT >= 112)
        /* 0.5 scaling - display every other pixel = 144x112 */
        next_dst=&lcd_framebuffer[YOFS*LCD_WIDTH+XOFS+ScreenHeight/2-1];
        for (y=(ScreenHeight/2)-1;y >= 0; y--) {
            dst = (next_dst--);
            for (x=0;x<ScreenWidth/2;x++) {
                *dst = palette[*(vbuf)];
                vbuf+=2;
                dst+=LCD_WIDTH;
            }
            vbuf+=ScreenWidth;
        }
#endif
#else  /* Greyscale LCDs */
#if (LCD_WIDTH >= 144) && (LCD_HEIGHT >= 112)
#if LCD_PIXELFORMAT == VERTICAL_PACKING
        /* 0.5 scaling - display every other pixel = 144x112 */
        next_dst=&lcd_framebuffer[YOFS/4*LCD_WIDTH+XOFS+ScreenHeight/2-1];
        for (y=(ScreenHeight/2)-1;y >= 0; y--) {
            dst = (next_dst--);
            for (x=0;x<ScreenWidth/8;x++) {
                *dst = (palette[*(vbuf+6)]<<6) | (palette[*(vbuf+4)] << 4) | (palette[*(vbuf+2)] << 2) | palette[*(vbuf)];
                vbuf+=8;
                dst+=LCD_WIDTH;
            }
            vbuf+=ScreenWidth;
        }
#endif /* Vertical Packing */
#endif /* Size >= 144x112 */
#endif /* Not Colour */
}

#endif
