/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Bertrik Sikken
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
 
#include <inttypes.h>

#include "config.h"
#include "s5l8700.h"
#include "lcd.h"

/*  LCD driver for the Meizu M6 SP using the CLCD controller in the S5L8700

    The Meizu M6 SP can have two different LCDs, the S6D0139 and another
    (yet unknown) type, the exact type is detected at run-time.

    Open issues:
    * LCD is currently in portrait mode instead of landscape mode
    * This LCD driver accesses the Rockbox framebuffer directly, so any changes
      to the framebuffer as shown directly even before lcd_update is called.
    * Sometimes part of the top of the screen appears at the bottom
    * The Meizu seems to hang after LCD initialisation
    * The driver for the S6D0139 LCD has not been tested yet
      
 */

/* LCD SPI connections */
#define LCD_SPI_SSn         (1<<1)  /* on PDAT7 */
#define LCD_SPI_MISO        (1<<2)  /* on PDAT3 */
#define LCD_SPI_MOSI        (1<<6)  /* on PDAT3 */
#define LCD_SPI_SCLK        (1<<7)  /* on PDAT3 */

/* LCD SPI communication definitions */
#define LCD_SPI_DEVICE_ID   (0x1D<<2)
#define LCD_SPI_INDEX_WRITE (LCD_SPI_DEVICE_ID|0)
#define LCD_SPI_STATUS_READ (LCD_SPI_DEVICE_ID|1)
#define LCD_SPI_DATA_WRITE  (LCD_SPI_DEVICE_ID|2)
#define LCD_SPI_DATA_READ   (LCD_SPI_DEVICE_ID|3)

#define LCD_TYPE1_ID        0x139   /* id for LCD type S6D0139 */

static int lcd_type = 0;

/* simple and crude delay */
static void lcd_delay(int count)
{
    volatile int i;
    for (i = 0; i < count; i++);
}

/* write 'data_out' of length 'bits' over SPI and return received data */
static unsigned int lcd_spi_transfer(int bits, unsigned int data_out)
{
    unsigned int data_in = 0;
    
    /* SSn active */
    PDAT7 &= ~LCD_SPI_SSn;
    lcd_delay(10);
    
    /* send and receive data */
    while (bits--) {
        /* CLK low */
        PDAT3 &= ~LCD_SPI_SCLK;
        
        /* set MOSI */
        if (data_out & (1 << bits)) {
            PDAT3 |= LCD_SPI_MOSI;
        }
        else {
            PDAT3 &= ~LCD_SPI_MOSI;
        }

        /* delay */
        lcd_delay(10);

        /* sample MISO */
        data_in <<= 1;
        if (PDAT3 & LCD_SPI_MISO) {
            data_in |= 1;
        }
        
        /* CLK high */
        PDAT3 |= LCD_SPI_SCLK;

        /* delay */
        lcd_delay(10);
    }

    /* SSn inactive */
    PDAT7 |= LCD_SPI_SSn;
    lcd_delay(10);
    
    return data_in;
}

/* initialize the lcd SPI port interface */
static void lcd_spi_init(void)
{
    /* configure SSn (P7.1) as output */
    PCON7 = (PCON7 & ~0x000000F0) | 0x00000010;
    
    /* configure MISO (P3.2) input, MOSI (P3.6) output, SCLK (P3.7) output */
    PCON3 = (PCON3 & ~0xFF000F00) | 0x11000000;
    
    /* set all outputs high */
    PDAT7 |= LCD_SPI_SSn;
    PDAT3 |= (LCD_SPI_MOSI | LCD_SPI_SCLK);
}

/* read LCD identification word over SPI */
static unsigned int lcd_read_reg(unsigned reg)
{
    unsigned int data;
    
    lcd_spi_transfer(24, (LCD_SPI_INDEX_WRITE << 16) | reg);
    data = lcd_spi_transfer(24, (LCD_SPI_DATA_READ << 16));
    return data & 0xFFFF;
}

/* write LCD register over SPI */
static void lcd_write_reg(unsigned char reg, unsigned int data)
{
    lcd_spi_transfer(24, (LCD_SPI_INDEX_WRITE << 16) | reg);
    lcd_spi_transfer(24, (LCD_SPI_DATA_WRITE << 16) | data);
}

/* enable/disable clock signals towards the lcd */
static void lcd_controller_power(bool on)
{
    if (on) {
        LCDCON1 |= 0x80003;
    }
    else {
        LCDCON1 &= ~0x80003;
    }
}

/* lcd init configuration for lcd type 1 */
static void lcd_init1(void)
{
    lcd_write_reg(0x07, 0x0000);    /* display control */
    lcd_write_reg(0x13, 0x0000);    /* power control 3 */
    lcd_delay(166670);
    
    lcd_write_reg(0x11, 0x3304);    /* power control 2 */
    lcd_write_reg(0x14, 0x1300);    /* power control 4 */
    lcd_write_reg(0x10, 0x1A20);    /* power control 1 */
    lcd_write_reg(0x13, 0x0040);    /* power control 3 */
    lcd_delay(833350);
    
    lcd_write_reg(0x13, 0x0060);    /* power control 3 */
    lcd_write_reg(0x13, 0x0070);    /* power control 3 */
    lcd_delay(3333400);
    
    lcd_write_reg(0x01, 0x0127);    /* driver output control */
    lcd_write_reg(0x02, 0x0700);    /* lcd driving waveform control */
    lcd_write_reg(0x03, 0x1030);    /* entry mode */
    lcd_write_reg(0x08, 0x0208);    /* blank period control 1 */
    lcd_write_reg(0x0B, 0x0620);    /* frame cycle control */
    lcd_write_reg(0x0C, 0x0110);    /* external interface control */
    lcd_write_reg(0x30, 0x0120);    /* gamma control 1 */
    lcd_write_reg(0x31, 0x0117);    /* gamma control 2 */
    lcd_write_reg(0x32, 0x0000);    /* gamma control 3 */
    lcd_write_reg(0x33, 0x0305);    /* gamma control 4 */
    lcd_write_reg(0x34, 0x0717);    /* gamma control 5 */
    lcd_write_reg(0x35, 0x0124);    /* gamma control 6 */
    lcd_write_reg(0x36, 0x0706);    /* gamma control 7 */
    lcd_write_reg(0x37, 0x0503);    /* gamma control 8 */
    lcd_write_reg(0x38, 0x1F03);    /* gamma control 9 */
    lcd_write_reg(0x39, 0x0009);    /* gamma control 10 */
    lcd_write_reg(0x40, 0x0000);    /* gate scan position */
    lcd_write_reg(0x41, 0x0000);    /* vertical scroll control */
    lcd_write_reg(0x42, 0x013F);    /* 1st screen driving position (end) */
    lcd_write_reg(0x43, 0x0000);    /* 1st screen driving position (start) */
    lcd_write_reg(0x44, 0x013F);    /* 2nd screen driving position (end) */
    lcd_write_reg(0x45, 0x0000);    /* 2nd screen driving position (start) */
    lcd_write_reg(0x46, 0xEF00);    /* horizontal window address */
    lcd_write_reg(0x47, 0x013F);    /* vertical window address (end) */
    lcd_write_reg(0x48, 0x0000);    /* vertical window address (start) */
    
    lcd_write_reg(0x07, 0x0015);    /* display control */
    lcd_delay(500000);
    lcd_write_reg(0x07, 0x0017);    /* display control */
    
    lcd_write_reg(0x20, 0x0000);    /* RAM address set (low part) */
    lcd_write_reg(0x21, 0x0000);    /* RAM address set (high part) */
    lcd_write_reg(0x22, 0x0000);    /* write data to GRAM */
}

/* lcd init configuration for lcd type 2 */
static void lcd_init2(void)
{
    lcd_write_reg(0x07, 0x0000);
    lcd_write_reg(0x12, 0x0000);
    lcd_delay(166670);

    lcd_write_reg(0x11, 0x000C);
    lcd_write_reg(0x12, 0x0A1C);
    lcd_write_reg(0x13, 0x0022);
    lcd_write_reg(0x14, 0x0000);

    lcd_write_reg(0x10, 0x7404);
    lcd_write_reg(0x11, 0x0738);
    lcd_write_reg(0x10, 0x7404);
    lcd_delay(833350);
    
    lcd_write_reg(0x07, 0x0009);
    lcd_write_reg(0x12, 0x065C);
    lcd_delay(3333400);
    
    lcd_write_reg(0x01, 0xE127);
    lcd_write_reg(0x02, 0x0300);
    lcd_write_reg(0x03, 0x1100);
    lcd_write_reg(0x08, 0x0008);
    lcd_write_reg(0x0B, 0x0000);
    lcd_write_reg(0x0C, 0x0000);
    lcd_write_reg(0x0D, 0x0007);
    lcd_write_reg(0x15, 0x0003);
    lcd_write_reg(0x16, 0x0014);
    lcd_write_reg(0x17, 0x0000);
    
    lcd_write_reg(0x30, 0x0503);    /* gamma? */
    lcd_write_reg(0x31, 0x0303);
    lcd_write_reg(0x32, 0x0305);
    lcd_write_reg(0x33, 0x0202);
    lcd_write_reg(0x34, 0x0204);
    lcd_write_reg(0x35, 0x0404);
    lcd_write_reg(0x36, 0x0402);
    lcd_write_reg(0x37, 0x0202);
    lcd_write_reg(0x38, 0x1000);
    lcd_write_reg(0x39, 0x1000);
    
    lcd_write_reg(0x07, 0x0009);
    lcd_delay(666680);

    lcd_write_reg(0x07, 0x0109);
    lcd_delay(666680);

    lcd_write_reg(0x07, 0x010B);
}

/* lcd enable for lcd type 1 */
static void lcd_enable1(bool on)
{
    if (on) {
        lcd_write_reg(0x00, 0x0001);    /* start oscillation */
        lcd_delay(166670);
        lcd_write_reg(0x10, 0x0000);    /* power control 1 */
        lcd_delay(166670);
        
        lcd_write_reg(0x11, 0x3304);    /* power control 2 */
        lcd_write_reg(0x14, 0x1300);    /* power control 4 */
        lcd_write_reg(0x10, 0x1A20);    /* power control 1 */
        lcd_write_reg(0x07, 0x0015);    /* display control */
        lcd_delay(500000);

        lcd_write_reg(0x20, 0x0000);    /* RAM address set (low part) */
        lcd_write_reg(0x21, 0x0000);    /* RAM address set (high part) */
        lcd_write_reg(0x22, 0x0000);    /* write data to GRAM */
    }
    else {
        lcd_write_reg(0x07, 0x0016);    /* display control */
        lcd_delay(166670 * 4);
        lcd_write_reg(0x07, 0x0004);    /* display control */
        lcd_delay(166670 * 4);

        lcd_write_reg(0x10, 0x1E21);    /* power control 1 */
        lcd_delay(166670);
    }
}

/* lcd enable for lcd type 2 */
static void lcd_enable2(bool on)
{
    if (on) {
        lcd_write_reg(0x10, 0x0400);
        lcd_delay(666680);

        lcd_write_reg(0x07, 0x0000);
        lcd_write_reg(0x12, 0x0000);
        lcd_delay(166670);

        lcd_write_reg(0x11, 0x000C);
        lcd_write_reg(0x12, 0x0A1C);
        lcd_write_reg(0x13, 0x0022);
        lcd_write_reg(0x14, 0x0000);
        lcd_write_reg(0x10, 0x7404);
        lcd_write_reg(0x11, 0x0738);
        lcd_write_reg(0x10, 0x7404);
        lcd_delay(833350);
        
        lcd_write_reg(0x07, 0x0009);
        lcd_write_reg(0x12, 0x065C);
        lcd_delay(3333400);
        
        lcd_write_reg(0x0B, 0x0000);
        lcd_write_reg(0x07, 0x0009);
        lcd_delay(666680);
        
        lcd_write_reg(0x07, 0x0109);
        lcd_delay(666680);
        
        lcd_write_reg(0x07, 0x010B);
    }
    else {
        lcd_write_reg(0x0B, 0x0109);
        lcd_write_reg(0x07, 0x0009);
        lcd_delay(666680);

        lcd_write_reg(0x07, 0x0008);
        lcd_delay(666680);

        lcd_write_reg(0x10, 0x0400);
        lcd_write_reg(0x10, 0x0401);
        lcd_delay(166670);
    }
}

/* turn both the lcd controller and the lcd itself on or off */
void lcd_enable(bool on)
{
    if (on) {
        /* enable controller clock */
        PWRCON &= ~(1 << 18);
        
        lcd_controller_power(true);
        lcd_delay(166670);
    }

    /* call type specific power function */
    if (lcd_type == 1) {
        lcd_enable1(on);
    }
    else {
        lcd_enable2(on);
    }
    
    if (!on) {
        lcd_controller_power(false);

        /* disable controller clock */
        PWRCON |= (1 << 18);
    }
}

/* initialise the lcd controller inside the s5l8700 */
static void lcd_controller_init(void)
{
    PWRCON &= ~(1 << 18);
    
    LCDCON1 =   (0 << 28) |     /* BURSTLEN */
                (0 << 19) |     /* DIVEN */
                (12 << 13) |    /* CLKVAL */
                (1 << 12) |     /* CLKDIR, 1=divided clock */
                (0 << 11) |     /* CLKSEL, 0=HCLK, 1=PLL */
                (5 << 6) |      /* BPPMODEF, 5=rgb565, 7=raw24 */
                (5 << 2) |      /* BPPMODEB, 5=rgb565, 7=raw24 */
                (0 << 0);       /* ENVID */
    LCDCON2 =   (2 << 9) |      /* PALFRM, 2=rgb565 palette */
                (1 << 7) |      /* IVCLK */
                (1 << 6) |      /* IHSYNC */
                (1 << 5) |      /* IVSYNC */
                (1 << 3);       /* IVDEN */
    LCDTCON1 = (lcd_type == 1) ? 0x070103 : 0x030303;
    LCDTCON2 = (lcd_type == 1) ? 0x070103 : 0x030703;
    LCDTCON3 = ((LCD_HEIGHT - 1) << 11) | (LCD_WIDTH - 1);
    LCDOSD1 = 0;
    LCDOSD2 = 0;
    LCDOSD3 = 0;

    LCDB1SADDR1 = 0;
    LCDB2SADDR1 = 0;
    LCDF1SADDR1 = 0;
    LCDF2SADDR1 = 0;
    LCDB1SADDR2 = 0;
    LCDB2SADDR2 = 0;
    LCDF1SADDR2 = 0;
    LCDF2SADDR2 = 0;
    LCDB1SADDR3 = 0;
    LCDB2SADDR3 = 0;
    LCDF1SADDR3 = 0;
    LCDF2SADDR3 = 0;

    LCDKEYCON = 0;
    LCDCOLVAL = 0;
    LCDBGCON = 0;
    LCDFGCON = 0;
    LCDDITHMODE = 0;

    LCDINTCON = 0;
}

void lcd_init_device(void)
{
    unsigned int lcd_id;
    uint32_t fb, fb_end, window;

    /* configure LCD SPI pins */
    lcd_spi_init();
    
    /* identify display through SPI */
    lcd_id = lcd_read_reg(0);
    lcd_type = (lcd_id == LCD_TYPE1_ID) ? 1 : 2;
    
    /* display specific init sequence */
    if (lcd_type == 1) {
        lcd_init1();
    }
    else {
        lcd_init2();
    }

    /* init LCD controller */
    lcd_controller_init();

    /* set framebuffer addresses */
    fb = (uint32_t) &lcd_framebuffer[0][0];
    fb_end = (uint32_t) &lcd_framebuffer[LCD_HEIGHT][0];
    window = 2 * LCD_WIDTH;

    LCDB1SADDR1 = fb;
    LCDB2SADDR1 = fb;
    LCDF1SADDR1 = fb;
    LCDF2SADDR1 = fb;

    LCDB1SADDR2 = fb_end;
    LCDB2SADDR2 = fb_end;
    LCDF1SADDR2 = fb_end;    LCDF2SADDR2 = fb_end;

    LCDB1SADDR3 = window;
    LCDB2SADDR3 = window;
    LCDF1SADDR3 = window;
    LCDF2SADDR3 = window;
    
    lcd_enable(true);

    /* configure LCD pins */
    PCON_ASRAM = 1;
}

void lcd_update_rect(int x, int y, int width, int height)
{
    /* not implemented yet, LCD controller accesses framebuffer directly */
    (void) x;
    (void) y;
    (void) width;
    (void) height;
}

void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

