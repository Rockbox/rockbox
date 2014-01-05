/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Bertrik Sikken
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
#include "system.h"
#include "kernel.h"

#include "lcd.h"
#include "lcd-target.h"

/* the detected lcd type (0 or 1) */
static int lcd_type;

#ifdef HAVE_LCD_ENABLE
/* whether the lcd is currently enabled or not */
static bool lcd_enabled;
#endif

/* initialises the host lcd hardware, returns the lcd type */
static int lcd_hw_init(void)
{
    /* configure SSP */
    bitset32(&CGU_PERI, CGU_SSP_CLOCK_ENABLE);
    SSP_CPSR = 4;           /* TODO: use AS3525_SSP_PRESCALER, OF uses 8 */
    SSP_CR0 =   (0 << 8) |  /* SCR, serial clock rate divider = 1 */
                (1 << 7) |  /* SPH, phase = 1 */
                (1 << 6) |  /* SPO, polarity = 1 */
                (0 << 4) |  /* FRF, frame format = motorola SPI */
                (7 << 0);   /* DSS, data size select = 8 bits */
    SSP_CR1 =   (1 << 3) |  /* SOD, slave output disable = 1 */
                (0 << 2) |  /* MS, master/slave = master */
                (1 << 1) |  /* SSE, synchronous serial port enabled = true */
                (0 << 0);   /* LBM, loopback mode = normal */
    SSP_IMSC &= ~0xF;       /* disable interrupts */
    SSP_DMACR &= ~0x3;      /* disable DMA */

    /* GPIO A3 is ??? but needs to be set */
    GPIOA_DIR |= (1 << 3);
    GPIOA_PIN(3) = (1 << 3);

    /* configure GPIO B2 (lcd D/C#) as output */
    GPIOB_DIR |= (1<<2);

    /* configure GPIO B3 (lcd type detect) as input */
    GPIOB_DIR &= ~(1<<3);
    
    /* configure GPIO A5 (lcd reset#) as output and perform lcd reset */
    GPIOA_DIR |= (1 << 5);
    GPIOA_PIN(5) = 0;
    sleep(HZ * 50/1000);
    GPIOA_PIN(5) = (1 << 5);

    /* detect lcd type on GPIO B3 */    
    return GPIOB_PIN(3) ? 1 : 0;
}

/* writes a command byte to the LCD */
static void lcd_write_cmd(uint8_t byte)
{
    /* wait until not busy */
    while (SSP_SR & (1<<4));

    /* LCD command mode */
    GPIOB_PIN(2) = 0;
    
    /* write data */
    SSP_DATA = byte;
    
    /* wait until not busy */
    while (SSP_SR & (1<<4));

    /* LCD data mode */
    GPIOB_PIN(2) = (1 << 2);
}

/* writes a data byte to the LCD */
static void lcd_write_dat(uint8_t data)
{
    /* wait while transmit FIFO */
    while (!(SSP_SR & (1<<1)));   

    /* write data */
    SSP_DATA = data;
}

/* writes both a command and data value to the lcd */
static void lcd_write(uint8_t cmd, uint8_t data)
{
    lcd_write_cmd(cmd);
    lcd_write_dat(data);
}

/*  Initialises lcd type 0
 *  This appears to be a WiseChip OLED display controlled by a SEPS114A.
 */
static void lcd_init_type0(void)
{
    lcd_write(0x01, 0x00);  /* SOFT_RESET */
    lcd_write(0x14, 0x01);  /* STANDBY_ON_OFF */
    sleep(1);   /* actually only 5 ms needed */

    lcd_write(0x14, 0x00);  /* STANDBY_ON_OFF */
    sleep(1);   /* actually only 5 ms needed */

    lcd_write(0x0F, 0x41);  /* ANALOG_CONTROL */
    lcd_write(0xEA, 0x0A);  /* ? */
    lcd_write(0xEB, 0x42);  /* ? */
    lcd_write(0x18, 0x08);  /* DISCHARGE_TIME */
    lcd_write(0x1A, 0x0B);  /* OSC_ADJUST */
    lcd_write(0x48, 0x03);  /* ROW_OVERLAP */
    lcd_write(0x30, 0x00);  /* DISPLAY_X1 */
    lcd_write(0x31, 0x5F);  /* DISPLAY_X2 */
    lcd_write(0x32, 0x00);  /* DISPLAY_Y1 */
    lcd_write(0x33, 0x5F);  /* DISPLAY_Y2 */
    lcd_write(0xE0, 0x10);  /* RGB_IF */
    lcd_write(0xE1, 0x00);  /* RGB_POL */
    lcd_write(0xE5, 0x80);  /* DISPLAY_MODE_CONTROL */
    lcd_write(0x0D, 0x00);  /* CPU_IF */
    lcd_write(0x1D, 0x01);  /* MEMORY_WRITE_READ */
    lcd_write(0x09, 0x00);  /* ROW_SCAN_DIRECTION */
    lcd_write(0x13, 0x00);  /* ROW_SCAN_MODE */
    lcd_write(0x16, 0x05);  /* PEAK_PULSE_DELAY */
    lcd_write(0x3A, 0x03);  /* PEAK_PULSE_WIDTH_R */
    lcd_write(0x3B, 0x03);  /* PEAK_PULSE_WIDTH_G */
    lcd_write(0x3C, 0x03);  /* PEAK_PULSE_WIDTH_B */
    lcd_write(0x3D, 0x45);  /* PRECHARGE_CURRENT_R */
    lcd_write(0x3E, 0x45);  /* PRECHARGE_CURRENT_G */
    lcd_write(0x3F, 0x45);  /* PRECHARGE_CURRENT_B */
    lcd_write(0x40, 0x62);  /* COLUMN_CURRENT_R */
    lcd_write(0x41, 0x3D);  /* COLUMN_CURRENT_G */
    lcd_write(0x42, 0x46);  /* COLUMN_CURRENT_B */
}

/* writes a table entry (for type 1 LCDs) */
static void lcd_write_nibbles(uint8_t val)
{
    lcd_write_dat((val >> 4) & 0x0F);
    lcd_write_dat((val >> 0) & 0x0F);
}

/* Initialises lcd type 1
 * This appears to be a Visionox OLED display, with an unknown controller
 */
static void lcd_init_type1(void)
{
    static const uint8_t curve[128] = {
        /* 5-bit curve */
        0, 5, 10, 15, 20, 25, 30, 35, 39, 43, 47, 51, 55, 59, 63, 67,
        71, 75, 79, 83, 87, 91, 95, 99, 103, 105, 109, 113, 117, 121, 123, 127,
        /* 6-bit curve */
        0, 2, 4, 6, 8, 10, 12, 16, 18, 24, 26, 28, 30, 32, 34, 36,
        38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68,
        70, 72, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 100, 102,
        104, 106, 108, 110, 112, 114, 116, 118, 120, 121, 122, 123, 124, 125, 126, 127,
        /* 5-bit curve */
        0, 5, 10, 15, 20, 25, 30, 35, 39, 43, 47, 51, 55, 59, 63, 67,
        71, 75, 79, 83, 87, 91, 93, 97, 101, 105, 109, 113, 117, 121, 124, 127
    };
    int i;

    lcd_write_cmd(0x02);
    lcd_write_dat(0x00);

    lcd_write_cmd(0x01);

    lcd_write_cmd(0x03);
    lcd_write_dat(0x00);

    lcd_write_cmd(0x04);
    lcd_write_dat(0x03);

    lcd_write_cmd(0x05);
    lcd_write_dat(0x00);    /* 0x08 results in BGR colour */

    lcd_write_cmd(0x06);
    lcd_write_dat(0x00);

    lcd_write_cmd(0x07);
    lcd_write_dat(0x00);
    lcd_write_dat(0x00);
    lcd_write_dat(0x04);
    lcd_write_dat(0x1F);
    lcd_write_dat(0x00);
    lcd_write_dat(0x00);
    lcd_write_dat(0x05);
    lcd_write_dat(0x0F);

    lcd_write_cmd(0x08);
    lcd_write_dat(0x01);

    lcd_write_cmd(0x09);
    lcd_write_dat(0x07);

    lcd_write_cmd(0x0A);
    lcd_write_nibbles(0);
    lcd_write_nibbles(LCD_WIDTH - 1);
    lcd_write_nibbles(0);
    lcd_write_nibbles(LCD_HEIGHT - 1);

    lcd_write_cmd(0x0B);
    lcd_write_dat(0x00);
    lcd_write_dat(0x00);
    lcd_write_dat(0x00);
    lcd_write_dat(0x00);

    lcd_write_cmd(0x0E);
    lcd_write_nibbles(0x42);
    lcd_write_nibbles(0x25);
    lcd_write_nibbles(0x3F);

    lcd_write_cmd(0x0F);
    lcd_write_dat(0x0A);
    lcd_write_dat(0x0A);
    lcd_write_dat(0x0A);

    lcd_write_cmd(0x1C);
    lcd_write_dat(0x08);

    lcd_write_cmd(0x1D);
    lcd_write_dat(0x00);
    lcd_write_dat(0x00);
    lcd_write_dat(0x00);

    lcd_write_cmd(0x1E);
    lcd_write_dat(0x05);

    lcd_write_cmd(0x1F);
    lcd_write_dat(0x00);

    lcd_write_cmd(0x30);
    lcd_write_dat(0x10);

    lcd_write_cmd(0x3A);
    for (i = 0; i < 128; i++) {
        lcd_write_nibbles(curve[i]);
    }

    lcd_write_cmd(0x3C);
    lcd_write_dat(0x00);

    lcd_write_cmd(0x3D);
    lcd_write_dat(0x00);
}

#ifdef HAVE_LCD_ENABLE
/* enables/disables the lcd */
void lcd_enable(bool on)
{
    if (on == lcd_enabled) {
        return;
    }

    if (lcd_type == 0) {
        if (on) {
            lcd_write(0x14, 0x00);  /* STANDBY_ON_OFF */
            lcd_write(0x02, 0x01);  /* DISP_ON_OFF */
            lcd_write(0xD2, 0x04);  /* SCREEN_SAVER_MODE */
            lcd_write(0xD0, 0x80);  /* SCREEN_SAVER_CONTROL */
            sleep(HZ * 100/1000);

            lcd_write(0xD0, 0x00);  /* SCREEN_SAVER_CONTROL */
        }
        else {
            lcd_write(0xD2, 0x05);
            lcd_write(0xD0, 0x80);
            sleep(HZ * 100/1000);

            lcd_write(0x02, 0x00);
            lcd_write(0xD0, 0x00);
            lcd_write(0x14, 0x01);
        }
    }
    else {
        if (on) {
            lcd_write_cmd(0x03);
            lcd_write_dat(0x00);

            lcd_write_cmd(0x02);
            lcd_write_dat(0x01);
        }
        else {
            lcd_write_cmd(0x02);
            lcd_write_dat(0x00);

            lcd_write_cmd(0x03);
            lcd_write_dat(0x01);
        }
    }
    
    lcd_enabled = on;
}

/* returns true if the lcd is enabled */
bool lcd_active(void)
{
    return lcd_enabled;
}
#endif /* HAVE_LCD_ENABLE */

/* initialises the lcd */
void lcd_init_device(void)
{
    lcd_type = lcd_hw_init();
    if (lcd_type == 0) {
        lcd_init_type0();
    }
    else {
        lcd_init_type1();
    }
    lcd_enable(true);
}

/* sets up the lcd to receive frame buffer data */
static void lcd_setup_rect(int x, int x_end, int y, int y_end)
{
    if (lcd_type == 0) {
        lcd_write(0x34, x);     /* MEM_X1 */
        lcd_write(0x35, x_end); /* MEM_X2 */
        lcd_write(0x36, y);     /* MEM_Y1 */
        lcd_write(0x37, y_end); /* MEM_Y2 */
        
        lcd_write_cmd(0x08);    /* DDRAM_DATA_ACCESS_PORT */
    }
    else {
        lcd_write_cmd(0x0A);
        lcd_write_nibbles(x);
        lcd_write_nibbles(x_end);
        lcd_write_nibbles(y);
        lcd_write_nibbles(y_end);
        
        lcd_write_cmd(0x0C);
    }
}

/* sets the brightness of the OLED */
void oled_brightness(int brightness)
{
    int r, g, b;

    if (lcd_type == 0) {
        r = 2 + 16*brightness;
        g = 1 + 10*brightness;
        b = 1 + (23*brightness)/2;
    
        lcd_write(0x40, r); /* COLUMN_CURRENT_R */
        lcd_write(0x41, g); /* COLUMN_CURRENT_G */
        lcd_write(0x42, b); /* COLUMN_CURRENT_B */
    }
    else {
        r = 6 + 10*brightness;
        g = 1 + 6*brightness;
        b = 3 + 10*brightness;
    
        lcd_write_cmd(0x0E);
        lcd_write_nibbles(r);
        lcd_write_nibbles(g);
        lcd_write_nibbles(b);
    }
}

/* Writes framebuffer data */
void lcd_write_data(const fb_data *data, int count)
{
    fb_data pixel;

    while (count--) {
        pixel = *data++;
        lcd_write_dat((pixel >> 8) & 0xFF);
        lcd_write_dat((pixel >> 0) & 0xFF);
    }
}

/* Updates a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    int row;
    int x_end = x + width;
    int y_end = y + height;

    /* check rectangle */
    if ((x >= LCD_WIDTH) || (x_end <= 0) || (y >= LCD_HEIGHT) || (y_end <= 0)) {
        /* rectangle is outside visible display, do nothing */
        return;
    }
    
    /* update entire horizontal strip for display type 0 (wisechip) */
    if (lcd_type == 0) {
        x = 0;
        x_end = 96;
    }
    
    /* correct rectangle (if necessary) */
    if (x < 0) {
        x = 0;
    }
    if (x_end > LCD_WIDTH) {
        x_end = LCD_WIDTH;
    }
    if (y < 0) {
        y = 0;
    }
    if (y_end > LCD_HEIGHT) {
        y_end = LCD_HEIGHT;
    }
    width = x_end - x;

    /* setup GRAM write window */
    lcd_setup_rect(x, x_end - 1, y, y_end - 1);

    /* write to GRAM */
    for (row = y; row < y_end; row++) {
        lcd_write_data(FBADDR(x,row), width);
    }
}

/* updates the entire lcd */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}
