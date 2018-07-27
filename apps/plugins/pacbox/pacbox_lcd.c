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
#include "pacbox_lcd.h"
#include "arcade.h"
#include "hardware.h"

void blit_display(fb_data* lcd_framebuffer, unsigned char* vbuf)
{
    fb_data* dst;
    fb_data* next_dst;
    int x,y;

#ifdef HAVE_LCD_COLOR
#if LCD_SCALE==100 && LCD_ROTATE==0
        /* Native resolution = 224x288 */
        (void)next_dst;
        dst=&lcd_framebuffer[YOFS*LCD_WIDTH+XOFS];
        for (y=0;y<ScreenHeight;y++) {
            for (x=0;x<ScreenWidth;x++) {
                *(dst++) = palette[*(vbuf++)];
            }
            dst += XOFS*2;
        }
#elif LCD_SCALE==100 && LCD_ROTATE==1
        /* Native resolution - rotated 90 degrees = 288x224 */
        next_dst=&lcd_framebuffer[YOFS*LCD_WIDTH+XOFS+ScreenHeight-1];
        for( y=ScreenHeight-1; y>=0; y-- ) {
            dst = (next_dst--);
            for( x=0; x<ScreenWidth; x++ ) {
                *dst = palette[*(vbuf++)];
                dst+=LCD_WIDTH;
            }
        }
#elif LCD_SCALE==100 && LCD_ROTATE==2
        /* Native resolution - rotated 270 degrees = 288x224 */
        next_dst=&lcd_framebuffer[(LCD_HEIGHT-YOFS)*LCD_WIDTH+XOFS];
        for( y=0; y<ScreenHeight; y++ ) {
            dst = (next_dst++);
            for( x=ScreenWidth-1; x>=0; x-- ) {
                *dst = palette[*(vbuf++)];
                dst-=LCD_WIDTH;
            }
        }
#elif LCD_SCALE==75 && LCD_ROTATE==1
        /* 0.75 scaling - display 3 out of 4 pixels - rotated = 216x168 
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
#elif LCD_SCALE==75 && LCD_ROTATE==0
        /* 0.75 scaling - display 3 out of 4 pixels - = 168x216
           Skipping pixel #2 out of 4 seems to give the most legible display 
         */
        (void)next_dst;
        dst=&lcd_framebuffer[YOFS*LCD_WIDTH+XOFS];
        for (y=0;y<ScreenHeight;y++) {
            if ((y & 3) != 1) {
                for (x=0;x<ScreenWidth;x++) {
                    if ((x & 3) == 1) { vbuf++; }
                    else {
                        *(dst++) = palette[*(vbuf++)];
                    }
                }
                dst += XOFS*2;
            } else {
                vbuf+=ScreenWidth;
            }
        }
#elif LCD_SCALE==50 && LCD_ROTATE==1
        /* 0.5 scaling - display every other pixel - rotated = 144x112 */
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
#elif LCD_SCALE==50 && LCD_ROTATE==0
        /* 0.5 scaling - display every other pixel
         * LCD_HEIGHT < 144: 112x144, crop to 112x128
         * else center vertically without clipping */
        (void)next_dst;
        dst=&lcd_framebuffer[YOFS*LCD_WIDTH+XOFS];
        
        /* Skip first YCLIP lines */
        vbuf+=ScreenWidth*YCLIP;
        
        /* Show regular screen */
        for (y=0;y<(ScreenHeight/2-YCLIP);y++) {
            for (x=0;x<ScreenWidth/2;x++) {
                *(dst++) = palette[*(vbuf)];
                vbuf+=2;
            }
            dst += XOFS*2;
            vbuf+=ScreenWidth;
        }
#elif LCD_SCALE==40 && LCD_ROTATE==1
        /* 0.4 scaling - rotated = 116x90 */
        /* show 2 out of 5 pixels: 1st and 3rd anf 4th merged together */
        next_dst=&lcd_framebuffer[XOFS*LCD_WIDTH+YOFS+ScreenHeight*2/5-1];
        for (y=(ScreenHeight*2/5)-1;y >= 0; y--) {
            dst = (next_dst--);
            for (x=0;x<ScreenWidth*2/5;x++) {
                *dst = palette[*(vbuf)] | palette[*(vbuf+ScreenWidth+1)];
                /* every odd row number merge 2 source lines as one */
                if (y & 1) *dst |= palette[*(vbuf+ScreenWidth*2)];
                vbuf+=2;
                dst+=LCD_WIDTH;

                x++;
                /* every odd column merge 2 colums together */
                *dst = palette[*(vbuf)] |  palette[*(vbuf+1)] |palette[*(vbuf+ScreenWidth+2)];
                if (y & 1) *dst |= palette[*(vbuf+ScreenWidth*2+1)];
                vbuf+=3;
                dst+=LCD_WIDTH;
            }
            vbuf+=ScreenWidth-1;
            if (y & 1) vbuf+=ScreenWidth;
        }
#elif LCD_SCALE==33 && LCD_ROTATE==0
        /* 1/3 scaling - display every third pixel - 75x96 */
        (void)next_dst;
        dst=&lcd_framebuffer[YOFS*LCD_WIDTH+XOFS];

        for (y=0;y<ScreenHeight/3;y++) {
            for (x=0;x<ScreenWidth/3;x++) {
                *(dst++) = palette[*(vbuf)]
                         | palette[*(vbuf+ScreenWidth+1)]
                         | palette[*(vbuf+ScreenWidth*2+2)];
                vbuf+=3;
            }
            dst += XOFS*2;
            vbuf+=ScreenWidth*2+2;
        }
#endif

#else  /* Greyscale LCDs */
#if LCD_SCALE==50 && LCD_ROTATE==1
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
#endif /* scale 50% rotated */
#endif /* Not Colour */
}
