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

extern bool lcd_on; /* lcd-memframe.c */

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

    /* configure GIO39, GIO34 as outputs */
    IO_GIO_DIR2 &= ~((1 << 7) /* GIO39 */ | (1 << 2) /* GIO34 */);
    
    IO_GIO_FSEL3 = (IO_GIO_FSEL3 & ~(0x300F)) |
                   (0x1000) /* GIO39 - FIELD_VENC */ |
                   (0x4);   /* GIO34 - PWM1 (brightness control) */

    /* OSD Clock = VENC Clock /2,
       CCD clock PCLK,
       VENC Clock from PLLA */
    IO_CLK_SEL1 = 0x3;

    /* Set VENC Clock Division to 11
       OF bootloader sets division to 8, vmlinux sets it to 11 */
    IO_CLK_DIV3 = (IO_CLK_DIV3 & ~(0x1F00)) | 0xB00;

    /* Enable OSD clock */
    bitset16(&IO_CLK_MOD1, CLK_MOD1_OSD);

    /* magic values based on OF bootloader initialization */
    /* Set DAC to powerdown mode (bit 2 to 1 in VMOD) */
    IO_VID_ENC_VMOD = 0x2014; /* OF sets 0x2010 */
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
    IO_VID_ENC_VMOD = 0x2015; /* OF sets 0x2011 */

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

    /* Enable Video Encoder and OSD clocks */
    bitset16(&IO_CLK_MOD1, CLK_MOD1_VENC | CLK_MOD1_OSD);

    /* Enable Video Encoder - RGB666, custom timing */
    IO_VID_ENC_VMOD = 0x2015;
    avr_hid_lcm_wake();
    lcd_on = true;
}

#ifdef LCD_USE_DMA
static void dma_lcd_copy_buffer_rect(int x, int y, int width, int height)
{
    char *dst, *src;

    /* Image buffer A is 4KW, every pixel is one Word */
    /* lines is maximum number of lines we can transfer in single run */
    int lines = 4096/width;
    if (lines > height)
        lines = height;

    /* Set source and destination addresses */
    dst = (char*)(FRAME + LCD_WIDTH*y + x);
    src = (char*)(FBADDR(x,y));
 
    /* Flush cache to memory */
    commit_dcache();

    /* Addresses are relative to start of SDRAM */
    src -= CONFIG_SDRAM_START;
    dst -= CONFIG_SDRAM_START;
    
    /* Enable Image Buffer clock */
    bitset16(&IO_CLK_MOD1, CLK_MOD1_IMGBUF);

    /* Use Image Buffer A for DMA */
    COP_BUF_MUX0 = (COP_BUF_MUX0 & 0xFFF0) | 0x0003;

    /* Setup buffer offsets and transfer width/height */
    COP_BUF_LOFST = width;
    COP_DMA_XNUM  = width;
    COP_DMA_YNUM  = lines;

    /* DMA: No byte SWAP, no transformation, data bus shift down 0 bit */
    COP_IMG_MODE &= 0xC0;

    /* Set the start address of buffer */
    COP_BUF_ADDR = 0x0000;
    
    /* Setup SDRAM stride */
    COP_SDEM_LOFST = LCD_WIDTH;

    do {
        int addr;

        addr = (int)src;
        addr >>= 1; /* Addresses are in 16-bit words */
        
        /* Setup the registers to initiate the read from SDRAM */
        COP_SDEM_ADDRH = addr >> 16;
        COP_SDEM_ADDRL = addr & 0xFFFF;
        
        /* Set direction and start */
        COP_DMA_CTRL = 0x0001;
        COP_DMA_CTRL |= 0x0002;

        /* Wait for read to finish */
        while (COP_DMA_CTRL & 0x02) {};
        
        addr = (int)dst;
        addr >>= 1;
        
        COP_SDEM_ADDRH = addr >> 16;
        COP_SDEM_ADDRL = addr & 0xFFFF;
        
        /* Set direction and start transfer */
        COP_DMA_CTRL = 0x0000;
        COP_DMA_CTRL |= 0x0002;
        
        /* Wait for the transfer to complete */
        while (COP_DMA_CTRL & 0x02) {};

        /* Decrease height, update pointers */
        src += (LCD_WIDTH << 1)*lines;
        dst += (LCD_WIDTH << 1)*lines;

        height -= lines;
        if (height < lines)
        {
            lines = height;
            COP_DMA_YNUM = height;
        }
    } while (height > 0);

    /* Disable Image Buffer clock */
    bitclr16(&IO_CLK_MOD1, CLK_MOD1_IMGBUF);
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
        __attribute__ ((section(".icode")));
void lcd_update_rect(int x, int y, int width, int height)
{
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

    dma_lcd_copy_buffer_rect(x, y, width, height);
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
#endif

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
