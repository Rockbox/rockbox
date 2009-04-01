/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
 *
 * Some of this is based on the Cowon A2 Firmware release:
 * http://www.cowonglobal.com/download/gnu/cowon_pmp_a2_src_1.59_GPL.tar.gz
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
#include "cpu.h"
#include "string.h"
#include "lcd.h"
#include "kernel.h"
#include "memory.h"
#include "system-target.h"
#include "lcd.h"
#include "lcd-target.h"

/* Copies a rectangle from one framebuffer to another. Can be used in
   single transfer mode with width = num pixels, and height = 1 which
   allows a full-width rectangle to be copied more efficiently. */
extern void lcd_copy_buffer_rect(fb_data *dst, const fb_data *src,
                                 int width, int height);

static bool lcd_on = true;
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
static bool lcd_powered = true;
#endif

/*
** These are imported from lcd-16bit.c
*/
extern unsigned fg_pattern;
extern unsigned bg_pattern;

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
bool lcd_active(void)
{
    return lcd_on;
}
#endif

#if defined(HAVE_LCD_SLEEP)
void lcd_sleep()
{
    if (lcd_powered)
    {
        /* "not powered" implies "disabled" */
        if (lcd_on)
            lcd_enable(false);
            
        /* Disabling these saves another ~15mA */
        IO_OSD_OSDWINMD0&=~(0x01);
		IO_VID_ENC_VMOD&=~(0x01);
    	
    	sleep(HZ/5);
    	
    	/* Disabling the LCD saves ~50mA */
    	IO_GIO_BITCLR2=1<<4;
    	lcd_powered=false;
    }
}
#endif

#if defined(HAVE_LCD_ENABLE)
void lcd_enable(bool state)
{
    if (state == lcd_on)
        return;

    if(state)
    {
        /* "enabled" implies "powered" */
        if (!lcd_powered)
        {
        	lcd_powered=true;
        	
        	IO_OSD_OSDWINMD0|=0x01;
			IO_VID_ENC_VMOD|=0x01;
    	
    		sleep(2);
            IO_GIO_BITSET2=1<<4;
            /* Wait long enough for a frame to be written - yes, it
             * takes awhile. */
            sleep(HZ/5);
        }

        lcd_on = true;
        lcd_update();
        lcd_activation_call_hook();
    }
    else 
    {
        lcd_on = false;
    }
}
#endif

/* LCD init - based on code from ingenient-bsp/bootloader/board/dm320/splash.c
 *  and code by Catalin Patulea from the M:Robe 500i linux port
 */
void lcd_init_device(void)
{
    unsigned int addr;
    
    /* Clear the Frame */
    memset16(FRAME, 0x0000, LCD_WIDTH*LCD_HEIGHT);

	/* Setup the LCD controller */
	IO_VID_ENC_VMOD=0x2015;
	IO_VID_ENC_VDCTL=0x2000;
	IO_VID_ENC_VDPRO=0x0000;
	IO_VID_ENC_SYNCTL=0x100E;
	IO_VID_ENC_HSPLS=1; /* HSYNC pulse width */
	IO_VID_ENC_VSPLS=1; /* VSYNC pulse width */
	
	/* These calculations support 640x480 and 320x240  */
	IO_VID_ENC_HINT=NATIVE_MAX_WIDTH+NATIVE_MAX_WIDTH/3;
	IO_VID_ENC_HSTART=NATIVE_MAX_WIDTH/6; /* Front porch */
	IO_VID_ENC_HVALID=NATIVE_MAX_WIDTH; /* Data valid */
	IO_VID_ENC_VINT=NATIVE_MAX_HEIGHT+7;
	IO_VID_ENC_VSTART=3;
	IO_VID_ENC_VVALID=NATIVE_MAX_HEIGHT;
	
	IO_VID_ENC_HSDLY=0x0000;
	IO_VID_ENC_VSDLY=0x0000;
	IO_VID_ENC_YCCTL=0x0000;
	IO_VID_ENC_RGBCTL=0x0000;
	IO_VID_ENC_RGBCLP=0xFF00;
	IO_VID_ENC_LNECTL=0x0000;
	IO_VID_ENC_CULLLNE=0x0000;
	IO_VID_ENC_LCDOUT=0x0000;
	IO_VID_ENC_BRTS=0x0000;
	IO_VID_ENC_BRTW=0x0000;
	IO_VID_ENC_ACCTL=0x0000;
	IO_VID_ENC_PWMP=0x0000;
	IO_VID_ENC_PWMW=0x0000;
	
	IO_VID_ENC_DCLKCTL=0x0800;
	IO_VID_ENC_DCLKPTN0=0x0001;


	/* Setup the display */
    IO_OSD_MODE=0x00ff;
    IO_OSD_VIDWINMD=0x0002;
    IO_OSD_OSDWINMD0=0x2001;
    IO_OSD_OSDWINMD1=0x0002;
    IO_OSD_ATRMD=0x0000;
    IO_OSD_RECTCUR=0x0000;

    IO_OSD_OSDWIN0OFST=(NATIVE_MAX_WIDTH*2) / 32;
    
    addr = ((int)FRAME-CONFIG_SDRAM_START) / 32;
    IO_OSD_OSDWINADH=addr >> 16;
    IO_OSD_OSDWIN0ADL=addr & 0xFFFF;
    
    IO_OSD_VIDWINADH=addr >> 16;
    IO_OSD_VIDWIN0ADL=addr & 0xFFFF;

    IO_OSD_BASEPX=IO_VID_ENC_HSTART;
    IO_OSD_BASEPY=IO_VID_ENC_VSTART;

    IO_OSD_OSDWIN0XP=0;
    IO_OSD_OSDWIN0YP=0;
  
    IO_OSD_OSDWIN0XL=NATIVE_MAX_WIDTH;
    IO_OSD_OSDWIN0YL=NATIVE_MAX_HEIGHT;

    /* Set pin 36 and 35 (LCD POWER and LCD RESOLUTION) to an output */
    IO_GIO_DIR2&=!(3<<3);
    
#if NATIVE_MAX_HEIGHT > 320
	/* Set LCD resolution to VGA */
    IO_GIO_BITSET2=1<<3;
#else
	/* Set LCD resolution to QVGA */
	IO_GIO_BITCLR2=1<<3;
#endif
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    register fb_data *dst, *src;

    if (!lcd_on)
        return;

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x; /* Clip right */
    if (x < 0)
        width += x, x = 0; /* Clip left */
    if (width <= 0)
        return; /* nothing left to do */

    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y; /* Clip bottom */
    if (y < 0)
        height += y, y = 0; /* Clip top */
    if (height <= 0)
        return; /* nothing left to do */

#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
    dst = (fb_data *)FRAME + LCD_WIDTH*y + x;
    src = &lcd_framebuffer[y][x];

    /* Copy part of the Rockbox framebuffer to the second framebuffer */
    if (width < LCD_WIDTH)
    {
        /* Not full width - do line-by-line */
        lcd_copy_buffer_rect(dst, src, width, height);
    }
    else
    {
        /* Full width - copy as one line */
        lcd_copy_buffer_rect(dst, src, LCD_WIDTH*height, 1);
    }
#else
    src = &lcd_framebuffer[y][x];
    
    register int xc, yc;
    register fb_data *start=FRAME + LCD_HEIGHT*(LCD_WIDTH-x-1) + y + 1;

    for(yc=0;yc<height;yc++)
    {
        dst=start+yc;
        for(xc=0; xc<width; xc++)
        {
            *dst=*src++;
            dst-=LCD_HEIGHT;
        }
        src+=x;
    }
#endif
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    if (!lcd_on)
        return;
#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
    lcd_copy_buffer_rect((fb_data *)FRAME, &lcd_framebuffer[0][0],
                         LCD_WIDTH*LCD_HEIGHT, 1);
#else
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
#endif
}

void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, 
                  int height) __attribute__ ((section(".icode")));
                                           
/* Performance function to blit a YUV bitmap directly to the LCD */
/* Show it rotated so the LCD_WIDTH is now the height */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    /* Caches for chroma data so it only need be recaculated every other
       line */
    unsigned char const * yuv_src[3];
    off_t z;

	/* Turn off the RGB buffer and enable the YUV buffer */
	IO_OSD_OSDWINMD0&=~(0x01);
	IO_OSD_VIDWINMD|=0x01;

    if (!lcd_on)
        return;
        
    /* y has to be at multiple of 2 or else it will mess up the HW (interleaving) */
    y &= ~1;

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;
    height>>=1;

    fb_data *dst = (fb_data*)FRAME + LCD_WIDTH*LCD_HEIGHT - x * LCD_WIDTH + y;

    z = stride*src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    {
        do
        {
        	register fb_data *c_dst=dst;
        	register int c_width=width;
        	unsigned char const * c_yuv_src[3];
        	c_yuv_src[0] = yuv_src[0];
        	c_yuv_src[1] = yuv_src[1];
        	c_yuv_src[2] = yuv_src[2];

        	do
        	{
        	/* This needs to be done in a block of 4 pixels */
        		*c_dst=*c_yuv_src[0]<<8 | *c_yuv_src[1];
        		*(c_dst+1)=*(c_yuv_src[0]+stride)<<8 | *c_yuv_src[2];
        		c_dst-=LCD_WIDTH;
        		c_yuv_src[0]++;
        		*c_dst=*c_yuv_src[0]<<8 | *c_yuv_src[1];
        		*(c_dst+1)=*(c_yuv_src[0]+stride)<<8 | *c_yuv_src[2];
        		c_dst-=LCD_WIDTH;
	            c_yuv_src[0]++;
	            
            	c_yuv_src[1]++;
            	c_yuv_src[2]++;
            	
        		c_width -= 2;
    		}
    		while (c_width > 0);
    		
    		yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            dst+=2;
        }
        while (--height > 0);
    }
}

void lcd_set_contrast(int val) {
  (void) val;
  // TODO:
}

void lcd_set_invert_display(bool yesno) {
  (void) yesno;
  // TODO:
}

void lcd_set_flip(bool yesno) {
  (void) yesno;
  // TODO:
}

