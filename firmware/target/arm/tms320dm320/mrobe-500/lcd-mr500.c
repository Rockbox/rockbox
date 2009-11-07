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
#include "kernel.h"
#include "memory.h"
#include "mmu-arm.h"
#include "system-target.h"
#include "lcd.h"
#include "lcd-target.h"
#include "dsp-target.h"
#include "dsp/ipc.h"

#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
#define LCD_USE_DMA
#elif defined(LCD_STRIDEFORMAT) && LCD_STRIDEFORMAT == VERTICAL_STRIDE
#define LCD_USE_DMA
#endif

/* Copies a rectangle from one framebuffer to another. Can be used in
   single transfer mode with width = num pixels, and height = 1 which
   allows a full-width rectangle to be copied more efficiently. */
extern void lcd_copy_buffer_rect(fb_data *dst, const fb_data *src,
                                 int width, int height);

#if defined(HAVE_LCD_SLEEP)
static bool lcd_on = true;
#endif

/*
** These are imported from lcd-16bit.c
*/
extern unsigned fg_pattern;
extern unsigned bg_pattern;

#if defined(HAVE_LCD_SLEEP)
bool lcd_active(void)
{
    return lcd_on;
}
#endif

#if defined(HAVE_LCD_SLEEP)
void lcd_sleep()
{
    if (lcd_on)
    {
        lcd_on = false;
		memset16(FRAME, 0xFFFF, LCD_WIDTH*LCD_HEIGHT);
		sleep(HZ/5);
        /* Disabling these saves another ~15mA */
        IO_OSD_OSDWINMD0    &= ~(0x01);
		IO_VID_ENC_VMOD     &= ~(0x01);
    	
    	/* Disabling the LCD saves ~50mA */
    	IO_GIO_BITCLR2=1<<4;
    }
}

void lcd_awake(void)
{
    /* "enabled" implies "powered" */
    if (!lcd_on)
    {
    	lcd_on=true;
    	IO_OSD_OSDWINMD0    |= 0x01;
		IO_VID_ENC_VMOD     |= 0x01;
	
		sleep(2);
        IO_GIO_BITSET2      = 1<<4;
        
        lcd_update();
        
        /* Wait long enough for a frame to be written */
        sleep(HZ/10);
        
        
        send_event(LCD_EVENT_ACTIVATION, NULL);
    }
}
#endif

/* Note this is expecting a screen size of 480x640 or 240x320, other screen
 * sizes need to be considered for fudge factors
 */
#define LCD_FUDGE       LCD_NATIVE_WIDTH%32

/* LCD init - based on code from ingenient-bsp/bootloader/board/dm320/splash.c
 *  and code by Catalin Patulea from the M:Robe 500i linux port
 */
void lcd_init_device(void)
{
    unsigned int addr;
    
    /* Pin 32 controls the LED above the LCD */
    IO_GIO_DIR2     &= ~(0x01);     /* output */
    IO_GIO_INV2     &= ~(0x01);     /* non-inverted */
    IO_GIO_FSEL2    &= ~(0x03<<14); /* normal pins */
    
    /* Turn the LED off */
    IO_GIO_BITCLR2 = 1;
    
    /* Clear the Frame */
    memset16(FRAME, 0x0000, LCD_WIDTH*LCD_HEIGHT);

	IO_OSD_OSDWINMD0    &= ~(0x0001);
	IO_OSD_VIDWINMD     &= ~(0x0001);

	/* Setup the LCD controller */
	IO_VID_ENC_VMOD     = 0x2014;
	IO_VID_ENC_VDCTL    = 0x2000;
	IO_VID_ENC_VDPRO    = 0x0000;
	IO_VID_ENC_SYNCTL   = 0x100E;
	IO_VID_ENC_HSPLS    = 1; /* HSYNC pulse width */
	IO_VID_ENC_VSPLS    = 1; /* VSYNC pulse width */
	
	/* These calculations support 640x480 and 320x240 (based on OF) */
	IO_VID_ENC_HINT     = LCD_NATIVE_WIDTH+LCD_NATIVE_WIDTH/3;
	IO_VID_ENC_HSTART   = LCD_NATIVE_WIDTH/6;   /* Back porch */
	IO_VID_ENC_HVALID   = LCD_NATIVE_WIDTH;     /* Data valid */
	IO_VID_ENC_VINT     = LCD_NATIVE_HEIGHT+8;
	IO_VID_ENC_VSTART   = 2;
	IO_VID_ENC_VVALID   = LCD_NATIVE_HEIGHT;
	
	IO_VID_ENC_HSDLY    = 0x0000;
	IO_VID_ENC_VSDLY    = 0x0000;
	IO_VID_ENC_YCCTL    = 0x0000;
	IO_VID_ENC_RGBCTL   = 0x0000;
	IO_VID_ENC_RGBCLP   = 0xFF00;
	IO_VID_ENC_LNECTL   = 0x0000;
	IO_VID_ENC_CULLLNE  = 0x0000;
	IO_VID_ENC_LCDOUT   = 0x0000;
	IO_VID_ENC_BRTS     = 0x0000;
	IO_VID_ENC_BRTW     = 0x0000;
	IO_VID_ENC_ACCTL    = 0x0000;
	IO_VID_ENC_PWMP     = 0x0000;
	IO_VID_ENC_PWMW     = 0x0000;
	
	IO_VID_ENC_DCLKPTN0 = 0x0001;

	/* Setup the display */
    IO_OSD_MODE         = 0x00ff;
    
    IO_OSD_ATRMD        = 0x0000;
    IO_OSD_RECTCUR      = 0x0000;
    
    IO_OSD_BASEPX       = IO_VID_ENC_HSTART;
    IO_OSD_BASEPY       = IO_VID_ENC_VSTART;
    
    addr                = ((int)FRAME-CONFIG_SDRAM_START) / 32;

    /* Setup the OSD windows */
    
    /* Used for 565 RGB */
    IO_OSD_OSDWINMD0    = 0x30C0;

    IO_OSD_OSDWIN0OFST  = LCD_NATIVE_WIDTH *2 / 32;
    
    IO_OSD_OSDWINADH    = addr >> 16;
    IO_OSD_OSDWIN0ADL   = addr & 0xFFFF;

    IO_OSD_OSDWIN0XP    = 0;
    IO_OSD_OSDWIN0YP    = 0;
    
    /* read from OF */
    IO_OSD_OSDWIN0XL    = LCD_NATIVE_WIDTH;
    IO_OSD_OSDWIN0YL    = LCD_NATIVE_HEIGHT;
    
    /* Unused */
    IO_OSD_OSDWINMD1    = 0x10C0;
    
#if LCD_NATIVE_WIDTH%32!=0
    IO_OSD_OSDWIN1OFST  = LCD_NATIVE_WIDTH / 32+1;
#else
    IO_OSD_OSDWIN1OFST  = LCD_NATIVE_WIDTH / 32;
#endif

    IO_OSD_OSDWIN1ADL   = addr & 0xFFFF;
    
    IO_OSD_OSDWIN1XP    = 0;
    IO_OSD_OSDWIN1YP    = 0;
    
    IO_OSD_OSDWIN1XL    = LCD_NATIVE_WIDTH;
    IO_OSD_OSDWIN1YL    = LCD_NATIVE_HEIGHT;
    
    IO_OSD_VIDWINMD     = 0x0000;
    
    addr                = ((int)FRAME2-CONFIG_SDRAM_START + 
                            2*(LCD_NATIVE_WIDTH*(LCD_NATIVE_HEIGHT-320)/2+
                                (LCD_NATIVE_WIDTH-240)/2))/ 32;
    
    /* This is a bit messy, the LCD transfers appear to happen in chunks of 32
     * pixels. (based on OF)
     */
#if LCD_NATIVE_WIDTH%32!=0
    IO_OSD_VIDWIN0OFST  = LCD_NATIVE_WIDTH * 2 / 32+1;
#else
    IO_OSD_VIDWIN0OFST  = LCD_NATIVE_WIDTH * 2 / 32;
#endif
    
    IO_OSD_VIDWINADH    = addr >> 16;
    IO_OSD_VIDWIN0ADL   = addr & 0xFFFF;
    
    IO_OSD_VIDWIN0XP    = 0;
    IO_OSD_VIDWIN0YP    = 0;
    
    IO_OSD_VIDWIN0XL    = LCD_NATIVE_WIDTH;
    IO_OSD_VIDWIN0YL    = LCD_NATIVE_HEIGHT;

    /* Set pin 36 and 35 (LCD POWER and LCD RESOLUTION) to an output */
    IO_GIO_DIR2         &= ~(3<<3);
    
#if LCD_NATIVE_HEIGHT > 320
	/* Set LCD resolution to VGA */
    IO_GIO_BITSET2      = 1<<3;
#else
	/* Set LCD resolution to QVGA */
	IO_GIO_BITCLR2      = 1<<3;
#endif

	IO_OSD_OSDWINMD0    |= 0x01;
	IO_VID_ENC_VMOD     |= 0x01;
}

#if defined(HAVE_LCD_MODES)
void lcd_set_mode(int mode)
{
    if(mode==LCD_MODE_YUV) {
        /* Turn off the RGB buffer and enable the YUV buffer with zoom */
        IO_OSD_OSDWINMD0    |= 0x04;
        IO_OSD_VIDWINMD     |= 0x01;
#if LCD_NATIVE_WIDTH > 240
        IO_OSD_VIDWINMD     |= (0x05<<2); /* This does a 2x zoom */
#endif
        memset16(FRAME2, 0x0080, LCD_NATIVE_HEIGHT*(LCD_NATIVE_WIDTH+LCD_FUDGE));
    } else if(mode==LCD_MODE_RGB565) {
        /* Turn on the RGB window, set it to 16 bit and turn YUV window off */
        IO_OSD_VIDWINMD     &= ~(0x01);
        IO_OSD_OSDWIN0OFST  =  LCD_NATIVE_WIDTH / 16;
        IO_OSD_OSDWINMD0    |= (1<<13);
        IO_OSD_OSDWINMD0    &= ~0x04;
        lcd_clear_display();
    } else if(mode==LCD_MODE_PAL256) {
#if LCD_NATIVE_WIDTH%32!=0
        IO_OSD_OSDWIN0OFST  = LCD_NATIVE_WIDTH / 32+1;
#else
        IO_OSD_OSDWIN0OFST  = LCD_NATIVE_WIDTH / 32;
#endif

        IO_OSD_VIDWINMD     &= ~(0x01);
        IO_OSD_OSDWINMD0    &= ~(1<<13);
        IO_OSD_OSDWINMD0    |= 0x01;
    }
}
#endif

#if defined(LCD_USE_DMA)
static void dma_start_transfer16(   char *src, int src_x, int src_y, int stride,
                                    int x, int y,
                                    int width, int height, int pix_width)
                                    __attribute__ ((section(".icode")));
#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
static void dma_start_transfer16(   char *src, int src_x, int src_y, int stride,
                                    int x, int y,
                                    int width, int height, int pix_width) {
    char *dst;
    
    /* Addresses are relative to start of SDRAM */
    src     =  src + (src_y*LCD_HEIGHT + src_x) * pix_width;
    dst     =   (char *)FRAME + (y * LCD_HEIGHT + x) * pix_width;
                    
    /* Flush the area that is being copied from. */
    clean_dcache_range(src, (stride*pix_width*width));
    
    /* Addresses are relative to start of SDRAM */
    src     -=  CONFIG_SDRAM_START;
    dst     -=  CONFIG_SDRAM_START;
    
    /* Enable Clocks */
    IO_CLK_MOD1     |= 1<<8;
    COP_CP_CLKC     |= 0x0001;

    /* ... */
    COP_BUF_MUX1    = 0x0005;
    /* Give the DMA access to the buffer */
    COP_BUF_MUX0    = 0x0663;
    
    /* Setup buffer offsets and transfer width/height */
    COP_BUF_LOFST   = width;
    COP_DMA_XNUM    = width;
    COP_DMA_YNUM    = 1;

    /* ... */
    COP_IMG_MODE    = 0x0000;
    
    /* Set the start address of buffer */
    COP_BUF_ADDR    = 0x0000;
    
    /* Setup SDRAM stride */
    COP_SDEM_LOFST  = stride;
    do {
        int addr;
        addr            = (int)src;
        addr            >>= 1; /* Addresses are in 16-bit words */
        
        /* Setup the registers to initiate the read from SDRAM */
        COP_SDEM_ADDRH  = addr >> 16;
        COP_SDEM_ADDRL  = addr & 0xFFFF;
        
        /* Set direction and start */
        COP_DMA_CTRL    = 0x0001;
        COP_DMA_CTRL    |= 0x0003;
        
        /* Wait for read to finish */
        while(COP_DMA_CTRL & 0x02) {};
        
        addr            = (int)dst;
        addr            >>= 1;
        
        COP_SDEM_ADDRH  = addr >> 16;
        COP_SDEM_ADDRL  = addr & 0xFFFF;
        
        /* Set direction and start transfer */
        COP_DMA_CTRL    = 0x0000;
        COP_DMA_CTRL    = 0x0002;
        
        /* Wait for the transfer to complete */
        while(COP_DMA_CTRL & 0x02) {};

        /* Decrease height, update pointers/counters */
        src     += (stride*pix_width);
        dst     += (stride*pix_width);
        height--;
    } while(height>0);
}
#else
static void dma_start_transfer16(   char *src, int src_x, int src_y, int stride,
                                    int x, int y,
                                    int width, int height, int pix_width) {
    char *dst;
    
    /* Calculate starting place */
    src     =  src + (src_x*LCD_HEIGHT + src_y) * pix_width;
    dst     =   (char *)FRAME + (LCD_HEIGHT*(LCD_WIDTH-1) - x * LCD_HEIGHT + y) 
                * pix_width;
    
    /* Flush the area that is being copied from. */
    clean_dcache_range(src, (stride*pix_width*width));
    
    /* Addresses are relative to start of SDRAM */
    src     -=  CONFIG_SDRAM_START;
    dst     -=  CONFIG_SDRAM_START;
    
    /* Enable Clocks */
    IO_CLK_MOD1     |= 1<<8;
    COP_CP_CLKC     |= 0x0001;

    /* ... */
    COP_BUF_MUX1    = 0x0005;
    /* Give the DMA access to the buffer */
    COP_BUF_MUX0    = 0x0663;
    
    /* Setup buffer offsets and transfer width/height */
    COP_BUF_LOFST   = height;
    COP_DMA_XNUM    = height;
    COP_DMA_YNUM    = 1;

    /* ... */
    COP_IMG_MODE    = 0x0000;
    
    /* Set the start address of buffer */
    COP_BUF_ADDR    = 0x0000;
    
    /* Setup SDRAM stride */
    COP_SDEM_LOFST  = stride;
    do {
        int addr;
        addr            = (int)src;
        addr            >>= 1; /* Addresses are in 16-bit words */
        
        /* Setup the registers to initiate the read from SDRAM */
        COP_SDEM_ADDRH  = addr >> 16;
        COP_SDEM_ADDRL  = addr & 0xFFFF;
        
        /* Set direction and start */
        COP_DMA_CTRL    = 0x0001;
        COP_DMA_CTRL    |= 0x0003;
        
        /* Wait for read to finish */
        while(COP_DMA_CTRL & 0x02) {};
        
        addr            = (int)dst;
        addr            >>= 1;
        
        COP_SDEM_ADDRH  = addr >> 16;
        COP_SDEM_ADDRL  = addr & 0xFFFF;
        
        /* Set direction and start transfer */
        COP_DMA_CTRL    = 0x0000;
        COP_DMA_CTRL    = 0x0002;
        
        /* Wait for the transfer to complete */
        while(COP_DMA_CTRL & 0x02) {};
        
        /* update the width, update pointers/counters */
        src     += (stride*pix_width);
        dst     -= (stride*pix_width);
        width--;
    } while(width>0);
}
#endif
#endif

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height) 
                __attribute__ ((section(".icode")));
void lcd_update_rect(int x, int y, int width, int height)
{
    if (!lcd_on)
        return;

    if ( (width | height) < 0)
        return; /* nothing left to do */

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x; /* Clip right */
    if (x < 0)
        width += x, x = 0; /* Clip left */

    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y; /* Clip bottom */
    if (y < 0)
        height += y, y = 0; /* Clip top */

#if CONFIG_ORIENTATION == SCREEN_PORTRAIT

#if defined(LCD_USE_DMA)
    dma_start_transfer16( (char *)lcd_framebuffer, x, y, LCD_WIDTH, 
        x, y, width, height, 2);
#else
    register fb_data *dst;

    dst = (fb_data *)FRAME + LCD_WIDTH*y + x;

    /* Copy part of the Rockbox framebuffer to the second framebuffer */
    if (width < LCD_WIDTH)
    {
        /* Not full width - do line-by-line */
        lcd_copy_buffer_rect(dst, &lcd_framebuffer[y][x], width, height);
    }
    else
    {
        /* Full width - copy as one line */
        lcd_copy_buffer_rect(dst, &lcd_framebuffer[y][x], LCD_WIDTH*height, 1);
    }
#endif

#else

#if   defined(LCD_STRIDEFORMAT) && LCD_STRIDEFORMAT == VERTICAL_STRIDE

#if defined(LCD_USE_DMA)
    dma_start_transfer16( (char *)lcd_framebuffer, x, y, LCD_HEIGHT, 
        x, y, width, height, 2);
#else
    fb_data *src;
    fb_data *dst;
    src     =  &lcd_framebuffer[0][0] + (x*LCD_HEIGHT + y);
    dst     =  FRAME + (LCD_HEIGHT*(LCD_WIDTH-1) - x * LCD_HEIGHT + y);

    while(width > 0) {
        memcpy(src, dst, height);
        src += LCD_HEIGHT;
        dst -= LCD_HEIGHT;
        width--;
    }
#endif

#else
    register fb_data *dst, *src;
    src = &lcd_framebuffer[y][x];

    dst=FRAME + (LCD_NATIVE_WIDTH*(LCD_NATIVE_HEIGHT-1)) 
        - LCD_NATIVE_WIDTH*x + y ;

    height--;
    do {
        register int c_width=width-1;
        register fb_data *c_dst=dst;
        
        do {
            *c_dst=*src++;
            c_dst-=LCD_NATIVE_WIDTH;
        } while(c_width--);
        
        src+=LCD_WIDTH-width;
        dst++;
    } while(height--);
#endif

#endif
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) __attribute__ ((section(".icode")));
void lcd_update(void)
{
    if (!lcd_on)
        return;
        
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
void lcd_blit_pal256(unsigned char *src, int src_x, int src_y, int x, int y,
	int width, int height) __attribute__ ((section(".icode")));
void lcd_blit_pal256(unsigned char *src, int src_x, int src_y, int x, int y,
	int width, int height)
{
#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
#if defined(LCD_USE_DMA)
//    char *dst=(char *)FRAME+x+y*(LCD_NATIVE_WIDTH+LCD_FUDGE);
    
    dma_start_transfer16(   src, src_x, src_y, LCD_WIDTH,
                            x, y, width, height, 1);
#else
    char *dst=(char *)FRAME+x+y*(LCD_NATIVE_WIDTH+LCD_FUDGE);

    src = src+src_x+src_y*LCD_WIDTH;
    while(height--)
    {
        memcpy(dst, src, width);

        dst = dst + ((LCD_WIDTH -x +LCD_FUDGE)); 
        src = src + (LCD_WIDTH - x);
    }
#endif
#else
    char *dst=(char *)FRAME
        + (LCD_NATIVE_WIDTH+LCD_FUDGE)*(LCD_NATIVE_HEIGHT-1)
        - (LCD_NATIVE_WIDTH+LCD_FUDGE)*x + y;
	
    src=src+src_x+src_y*width;

    while(height--)
    {
        register char *c_src=src;
        register char *c_dst=dst;
        register int c_width=width;

        while (c_width--)
        {
            *c_dst = *c_src++;
            c_dst -= (LCD_NATIVE_WIDTH+LCD_FUDGE);
        } 

        dst++;
        src+=width;
    }
#endif
}

void lcd_pal256_update_pal(fb_data *palette)
{
	unsigned char i;
	for(i=0; i< 255; i++) 
	{
	    int y, cb, cr;
		unsigned char r=RGB_UNPACK_RED_LCD(palette[i])<<3;
		unsigned char g=RGB_UNPACK_GREEN_LCD(palette[i])<<2;
		unsigned char b=RGB_UNPACK_BLUE_LCD(palette[i])<<3;
        
        y = ((77 * r + 150 * g + 29 * b) >> 8);        cb = ((-43 * r - 85 * g + 128 * b) >> 8) + 128;
        cr = ((128 * r - 107 * g - 21 * b) >> 8) + 128;
		
        while(IO_OSD_MISCCTL&0x08)
        {};
		
		/* Write in y and cb */
       	IO_OSD_CLUTRAMYCB= ((unsigned char)y << 8) | (unsigned char)cb;
		
		/* Write in the index and cr */
       	IO_OSD_CLUTRAMCR=((unsigned char)cr << 8) | i;
	}
}
#endif
	
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
    register unsigned char const * yuv_src[3];

    if (!lcd_on)
        return;
	
    /* y has to be on a 16 pixel boundary */
    y &= ~0xF;

    if(     ((y | x | height | width ) < 0) 
            || y>LCD_NATIVE_HEIGHT || x>LCD_NATIVE_WIDTH )
        return;

    if(y+height>LCD_NATIVE_WIDTH)
    {
        height=LCD_NATIVE_WIDTH-y;
    }
    if(x+width>LCD_NATIVE_HEIGHT)
    {
        width=LCD_NATIVE_HEIGHT-x;
    }

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;
    height>>=1;

    fb_data * dst = FRAME2 
        + ((LCD_NATIVE_WIDTH+LCD_FUDGE)*(LCD_NATIVE_HEIGHT-1)) 
        - (LCD_NATIVE_WIDTH+LCD_FUDGE)*x + y ;

    /* Scope z */
    {
        off_t z;
        z = stride*src_y;
        yuv_src[0] = src[0] + z + src_x;
        yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
        yuv_src[2] = src[2] + (yuv_src[1] - src[1]);
    }

    register int cbcr_remain=(stride>>1)-(width>>1);
    register int y_remain=(stride<<1)-width;
    do
    {
    	register fb_data *c_dst=dst;
    	register int c_width=width;

    	do
    	{
    	    /* This needs to be done in a block of 4 pixels */
    	    
    		*c_dst=*yuv_src[0]<<8 | *yuv_src[1];
    		*(c_dst+1)=*(yuv_src[0]+stride)<<8 | *yuv_src[2];
    		c_dst-=(LCD_NATIVE_WIDTH+LCD_FUDGE);
    		
    		yuv_src[0]++;
    		
    		*c_dst=*yuv_src[0]<<8 | *yuv_src[1];
    		*(c_dst+1)=*(yuv_src[0]+stride)<<8 | *yuv_src[2];
    		c_dst-=(LCD_NATIVE_WIDTH+LCD_FUDGE);
    		
            yuv_src[0]++;
            
        	yuv_src[1]++;
        	yuv_src[2]++;
        	
    		c_width -= 2;
		}
		while (c_width > 0);
		
		yuv_src[0] += y_remain; /* Skip down two luma lines-width */
        yuv_src[1] += cbcr_remain; /* Skip down one chroma line-width/2 */
        yuv_src[2] += cbcr_remain;
        dst+=2;
    }
    while (--height > 0);
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

