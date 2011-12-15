/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2011 by Tomasz Moń
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

#include <sys/types.h>
#include "config.h"
#include "cpu.h"
#include "string.h"
#include "kernel.h"
#include "system.h"
#include "system-target.h"
#include "lcd.h"
#include "lcd-target.h"
#include "avr-sansaconnect.h"

/* Copies a rectangle from one framebuffer to another. Can be used in
   single transfer mode with width = num pixels, and height = 1 which
   allows a full-width rectangle to be copied more efficiently. */
extern void lcd_copy_buffer_rect(fb_data *dst, const fb_data *src,
                                 int width, int height);

static bool lcd_on = true;

bool lcd_active(void)
{
    return lcd_on;
}

#if defined(HAVE_LCD_SLEEP)
void lcd_sleep(void)
{
    if (lcd_on)
    {
        lcd_on = false;
        avr_hid_lcm_sleep();
        sleep(HZ/20);

        /* disable video encoder */
        bitclr16(&IO_VID_ENC_VMOD, 0x01);

        sleep(HZ/20);

        /* disable video encoder clock */
        bitclr16(&IO_CLK_MOD1, CLK_MOD1_VENC);
    }
}

void lcd_awake(void)
{
    if (!lcd_on)
    {
        lcd_on = true;
        /* enable video encoder clock */
        bitset16(&IO_CLK_MOD1, CLK_MOD1_VENC); 

        /* enable video encoder */
        bitset16(&IO_VID_ENC_VMOD, 0x01);

        avr_hid_lcm_wake();

        send_event(LCD_EVENT_ACTIVATION, NULL);

        lcd_update();
    }
}
#endif

void lcd_init_device(void)
{
    unsigned int addr;
 
    /* Disable Video Encoder clock */
    bitclr16(&IO_CLK_MOD1, CLK_MOD1_VENC);

    /* configure GIO39, GIO34 and GIO33 as outputs */
    IO_GIO_DIR2 &= ~((1 << 7) /* GIO39 */ | (1 << 2) /* GIO34 */ |
                     (1 << 1) /* GIO33 */);
    
    IO_GIO_FSEL3 = (IO_GIO_FSEL3 & ~(0x300F)) |
                   (0x1000) /* GIO39 - FIELD_VENC */ |
                   (0x3)    /* GIO33 - CLKOUT1B (bootloader does this) */ |
                   (0x4);   /* GIO34 - PWM1 (brightness control) */

    /* OSD Clock = VENC Clock /2,
       CCD clock PCLK,
       VENC Clock from PLLA */
    IO_CLK_SEL1 = 0x3;

    /* Set VENC Clock Division to 11
       OF bootloader sets division to 8, vmlinux sets it to 11 */
    IO_CLK_DIV3 = (IO_CLK_DIV3 & ~(0x1F00)) | 0xB00;

    /* Enable DAC and OSD clocks */
    bitset16(&IO_CLK_MOD1, CLK_MOD1_DAC | CLK_MOD1_OSD);

    /* magic values based on OF bootloader initialization */
    IO_VID_ENC_VMOD = 0x2010;
    IO_VID_ENC_VDPRO = 0x80;
    IO_VID_ENC_HSPLS = 0x4;
    IO_VID_ENC_HINT = 0x4B0;
    IO_VID_ENC_HSTART = 0x88;
    IO_VID_ENC_HVALID = 0x3C0;
    IO_VID_ENC_HSDLY = 0;
    IO_VID_ENC_VSPLS = 0x2;
    IO_VID_ENC_VINT = 0x152;
    IO_VID_ENC_VSTART = 0x6;
    IO_VID_ENC_VVALID = 0x140;
    IO_VID_ENC_VSDLY = 0;
    IO_VID_ENC_DCLKCTL = 0x3;
    IO_VID_ENC_DCLKPTN0 = 0xC;
    IO_VID_ENC_VDCTL = 0x6000;
    IO_VID_ENC_SYNCTL = 0x2;
    IO_VID_ENC_LCDOUT = 0x101;
    IO_VID_ENC_VMOD = 0x2011;

    /* Copy Rockbox frame buffer to the second framebuffer */
    lcd_update();

    avr_hid_sync();
    avr_hid_lcm_power_on();

    /* set framebuffer address - OF sets RAM start address to 0x1000000 */
    addr = ((int)FRAME-CONFIG_SDRAM_START)/32;

    IO_OSD_OSDWINADH    = addr >> 16;
    IO_OSD_OSDWIN0ADL   = addr & 0xFFFF;

    IO_OSD_BASEPX = 0x44;
    IO_OSD_BASEPY = 0x6;
    IO_OSD_OSDWIN0XP = 0;
    IO_OSD_OSDWIN0YP = 0;
    IO_OSD_OSDWIN0XL = LCD_WIDTH*2;  /* OF bootloader sets 480 */
    IO_OSD_OSDWIN0YL = LCD_HEIGHT;   /* OF bootloader sets 320 */
    IO_OSD_OSDWIN0OFST = 0xF;
    IO_OSD_OSDWINMD0 = 0x25FB;/* OF bootloader sets 25C3,
                                 vmlinux changes this to 0x25FB */
    IO_OSD_VIDWINMD = 0; /* disable video windows (OF sets 0x03) */

    IO_OSD_OSDWINMD1 = 0; /* disable OSD window 1 */

    /* Enable DAC, Video Encoder and OSD clocks */
    bitset16(&IO_CLK_MOD1, CLK_MOD1_DAC | CLK_MOD1_VENC | CLK_MOD1_OSD);

    /* Enable Video Encoder - RGB666, custom timing */
    IO_VID_ENC_VMOD = 0x2011;
    avr_hid_lcm_wake();
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
        __attribute__ ((section(".icode")));
void lcd_update_rect(int x, int y, int width, int height)
{
    register fb_data *dst, *src;

    if (!lcd_on)
        return;

    if ((width | height) < 0)
        return; /* Nothing left to do */

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x; /* Clip right */
    if (x < 0)
        width += x, x = 0; /* Clip left */

    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y; /* Clip bottom */
    if (y < 0)
        height += y, y = 0; /* Clip top */

    dst = FRAME + LCD_WIDTH*y + x;
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

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_lines(fb_data *dst,
                                   unsigned char chroma_buf[LCD_HEIGHT/2*3],
                                   unsigned char const * const src[3],
                                   int width, int stride);

/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    /* Caches for chroma data so it only need be recalculated every other
       line */
    unsigned char chroma_buf[LCD_HEIGHT/2*3]; /* 480 bytes */
    unsigned char const * yuv_src[3];
    off_t z;

    if (!lcd_on)
        return;

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;
    height >>= 1;

    fb_data *dst = (fb_data*)FRAME + x * LCD_WIDTH + (LCD_WIDTH - y) - 1;

    z = stride*src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    do
    {
        lcd_write_yuv420_lines(dst, chroma_buf, yuv_src, width,
        stride);

        yuv_src[0] += stride << 1; /* Skip down two luma lines */
        yuv_src[1] += stride >> 1; /* Skip down one chroma line */
        yuv_src[2] += stride >> 1;
        dst -= 2;
    }
    while (--height > 0);
}

