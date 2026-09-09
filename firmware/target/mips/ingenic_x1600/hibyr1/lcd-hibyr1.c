/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

#include "lcd.h"
#include "system.h"
#include "kernel.h"
#include "dpu-x1600.h"
#include "clk-x1600.h"
#include "gpio-x1600.h"
#include <stdint.h>

/* ST7701S panel, 480x800 RGB666. Initialisation sequence from the vendor
 * module. */

/* From the DPU's live registers and FBIOGET_VSCREENINFO */
#define R1_PIXCLOCK_HZ      28303248
#define R1_HSYNC_LEN        24
#define R1_LEFT_MARGIN      24      /* horizontal back porch  */
#define R1_RIGHT_MARGIN     24      /* horizontal front porch */
#define R1_VSYNC_LEN        5
#define R1_UPPER_MARGIN     8       /* vertical back porch  */
#define R1_LOWER_MARGIN     14      /* vertical front porch */
#define R1_HTOTAL           (R1_HSYNC_LEN + R1_LEFT_MARGIN + \
                             LCD_WIDTH + R1_RIGHT_MARGIN)
#define R1_VTOTAL           (R1_VSYNC_LEN + R1_UPPER_MARGIN + \
                             LCD_HEIGHT + R1_LOWER_MARGIN)

/* MPLL/50 supplies the 28 MHz pixel clock. */
#define R1_LCD_CLKSRC       X1600_CLK_MPLL

/* Vendor panel initialisation encoded as 9-bit SPI words; bit 8
 * distinguishes data from commands. */
#define ST_CMD(x)       (x)             /* 9-bit word, D/C = 0 */
#define ST_DAT(x)       (0x100 | (x))   /* 9-bit word, D/C = 1 */
#define ST_DELAY(ms)    (0x8000 | (ms))
#define ST_END          0xffff

static const uint16_t st7701s_init_seq[] = {
    /* --- command2 BK3 ------------------------------------------- */
    ST_CMD(0xff), ST_DAT(0x77), ST_DAT(0x01), ST_DAT(0x00),
                  ST_DAT(0x00), ST_DAT(0x13),
    ST_CMD(0xef), ST_DAT(0x08),

    /* --- command2 BK0: panel geometry, porches, gamma ----------- */
    ST_CMD(0xff), ST_DAT(0x77), ST_DAT(0x01), ST_DAT(0x00),
                  ST_DAT(0x00), ST_DAT(0x10),
    /* LNESET: 800 lines */
    ST_CMD(0xc0), ST_DAT(0x63), ST_DAT(0x00),
    /* The panel's own VBP/VFP, not the DPU's */
    ST_CMD(0xc1), ST_DAT(0x09), ST_DAT(0x0c),
    /* INVSEL */
    ST_CMD(0xc2), ST_DAT(0x07), ST_DAT(0x08),
    ST_CMD(0xcc), ST_DAT(0x30),
    /* PVGAMCTRL - positive voltage gamma, 16 params */
    ST_CMD(0xb0), ST_DAT(0x00), ST_DAT(0x0d), ST_DAT(0x14), ST_DAT(0x0d),
                  ST_DAT(0x11), ST_DAT(0x07), ST_DAT(0x04), ST_DAT(0x08),
                  ST_DAT(0x08), ST_DAT(0x20), ST_DAT(0x05), ST_DAT(0x14),
                  ST_DAT(0x12), ST_DAT(0x25), ST_DAT(0x2d), ST_DAT(0x1c),
    /* NVGAMCTRL - negative voltage gamma, 16 params */
    ST_CMD(0xb1), ST_DAT(0x00), ST_DAT(0x0c), ST_DAT(0x14), ST_DAT(0x0d),
                  ST_DAT(0x11), ST_DAT(0x06), ST_DAT(0x03), ST_DAT(0x08),
                  ST_DAT(0x08), ST_DAT(0x1f), ST_DAT(0x05), ST_DAT(0x14),
                  ST_DAT(0x12), ST_DAT(0x25), ST_DAT(0x2e), ST_DAT(0x1c),

    /* --- command2 BK1: power, VCOM, gate driver ----------------- */
    ST_CMD(0xff), ST_DAT(0x77), ST_DAT(0x01), ST_DAT(0x00),
                  ST_DAT(0x00), ST_DAT(0x11),
    ST_CMD(0xb0), ST_DAT(0x58),      /* VRHA  */
    ST_CMD(0xb1), ST_DAT(0x4a),      /* VCOM  */
    ST_CMD(0xb2), ST_DAT(0x87),      /* VGH   */
    ST_CMD(0xb3), ST_DAT(0x80),      /* TESTCMD */
    ST_CMD(0xb5), ST_DAT(0x4c),      /* VGL   */
    ST_CMD(0xb7), ST_DAT(0x8a),      /* PWCTRL1 */
    ST_CMD(0xb8), ST_DAT(0x21),      /* PWCTRL2 */
    ST_CMD(0xc0), ST_DAT(0x03),
    ST_CMD(0xc1), ST_DAT(0x78),      /* SPD1 */
    ST_CMD(0xc2), ST_DAT(0x78),      /* SPD2 */
    ST_CMD(0xd0), ST_DAT(0x88),      /* MIPISET1 */

    ST_CMD(0xe0), ST_DAT(0x00), ST_DAT(0x00), ST_DAT(0x02),
    ST_CMD(0xe1), ST_DAT(0x01), ST_DAT(0xa0), ST_DAT(0x03), ST_DAT(0xa0),
                  ST_DAT(0x02), ST_DAT(0xa0), ST_DAT(0x04), ST_DAT(0xa0),
                  ST_DAT(0x00), ST_DAT(0x44), ST_DAT(0x44),
    ST_CMD(0xe2), ST_DAT(0x00), ST_DAT(0x00), ST_DAT(0x00), ST_DAT(0x00),
                  ST_DAT(0x00), ST_DAT(0x00), ST_DAT(0x00), ST_DAT(0x00),
                  ST_DAT(0x00), ST_DAT(0x00), ST_DAT(0x00), ST_DAT(0x00),
    ST_CMD(0xe3), ST_DAT(0x00), ST_DAT(0x00), ST_DAT(0x33), ST_DAT(0x33),
    ST_CMD(0xe4), ST_DAT(0x44), ST_DAT(0x44),
    ST_CMD(0xe5), ST_DAT(0x01), ST_DAT(0x26), ST_DAT(0xa0), ST_DAT(0xa0),
                  ST_DAT(0x03), ST_DAT(0x28), ST_DAT(0xa0), ST_DAT(0xa0),
                  ST_DAT(0x05), ST_DAT(0x2a), ST_DAT(0xa0), ST_DAT(0xa0),
                  ST_DAT(0x07), ST_DAT(0x2c), ST_DAT(0xa0), ST_DAT(0xa0),
    ST_CMD(0xe6), ST_DAT(0x00), ST_DAT(0x00), ST_DAT(0x33), ST_DAT(0x33),
    ST_CMD(0xe7), ST_DAT(0x44), ST_DAT(0x44),
    ST_CMD(0xe8), ST_DAT(0x02), ST_DAT(0x26), ST_DAT(0xa0), ST_DAT(0xa0),
                  ST_DAT(0x04), ST_DAT(0x28), ST_DAT(0xa0), ST_DAT(0xa0),
                  ST_DAT(0x06), ST_DAT(0x2a), ST_DAT(0xa0), ST_DAT(0xa0),
                  ST_DAT(0x08), ST_DAT(0x2c), ST_DAT(0xa0), ST_DAT(0xa0),
    ST_CMD(0xeb), ST_DAT(0x00), ST_DAT(0x00), ST_DAT(0xe4), ST_DAT(0xe4),
                  ST_DAT(0x44), ST_DAT(0x00), ST_DAT(0x40),
    ST_CMD(0xed), ST_DAT(0xff), ST_DAT(0xf7), ST_DAT(0x65), ST_DAT(0x4f),
                  ST_DAT(0x0b), ST_DAT(0xa1), ST_DAT(0xcf), ST_DAT(0xff),
                  ST_DAT(0xff), ST_DAT(0xfc), ST_DAT(0x1a), ST_DAT(0xb0),
                  ST_DAT(0xf4), ST_DAT(0x56), ST_DAT(0x7f), ST_DAT(0xff),
    ST_CMD(0xef), ST_DAT(0x08), ST_DAT(0x08), ST_DAT(0x08), ST_DAT(0x45),
                  ST_DAT(0x3f), ST_DAT(0x54),

    /* --- command2 BK3 again: power-up, pixel format, scan order -- */
    ST_CMD(0xff), ST_DAT(0x77), ST_DAT(0x01), ST_DAT(0x00),
                  ST_DAT(0x00), ST_DAT(0x13),
    ST_CMD(0xe8), ST_DAT(0x00), ST_DAT(0x0e),
    /* COLMOD 0x66 = RGB666, matching the parallel bus wiring */
    ST_CMD(0x3a), ST_DAT(0x66),
    /* MADCTL 0x08 = BGR order, no row/column exchange or mirroring */
    ST_CMD(0x36), ST_DAT(0x08),
    /* SLPOUT. BELOW the datasheet's 120 ms -- raise it if frame 1 is bad. */
    ST_CMD(0x11),
    ST_DELAY(100),
    ST_CMD(0xe8), ST_DAT(0x00), ST_DAT(0x0c),
    ST_DELAY(20),
    ST_CMD(0xe8), ST_DAT(0x00), ST_DAT(0x00),
    ST_CMD(0xe6), ST_DAT(0x16), ST_DAT(0x7c),

    /* --- back to the user command set, display on --------------- */
    ST_CMD(0xff), ST_DAT(0x77), ST_DAT(0x01), ST_DAT(0x00),
                  ST_DAT(0x00), ST_DAT(0x00),
    ST_CMD(0x29),

    ST_END,
};

/* The vendor just cuts bldo1/bldo2; DISPOFF+SLPIN is a guess */
static const uint16_t st7701s_sleep_seq[] = {
    ST_CMD(0x28),           /* DISPOFF */
    ST_DELAY(20),
    ST_CMD(0x10),           /* SLPIN */
    ST_DELAY(120),
    ST_END,
};

static const uint16_t st7701s_wake_seq[] = {
    ST_CMD(0x11),           /* SLPOUT */
    ST_DELAY(120),
    ST_CMD(0x29),           /* DISPON */
    ST_END,
};

/* Bit-banged 9-bit SPI */

/* Unverified bit period: the vendor's spi-gpio speed was never read back */
#define LCD_SPI_HALF_US 1

static void lcd_spi_write9(unsigned word)
{
    /* CS is framed per 9-bit word, as the vendor's spi-gpio does */
    gpio_set_level(GPIO_LCD_SPI_CS, 0);
    udelay(LCD_SPI_HALF_US);

    for(int i = 8; i >= 0; --i) {
        gpio_set_level(GPIO_LCD_SPI_SCK, 0);
        gpio_set_level(GPIO_LCD_SPI_MOSI, (word >> i) & 1);
        udelay(LCD_SPI_HALF_US);
        /* data is latched on the rising edge */
        gpio_set_level(GPIO_LCD_SPI_SCK, 1);
        udelay(LCD_SPI_HALF_US);
    }

    gpio_set_level(GPIO_LCD_SPI_CS, 1);
    udelay(LCD_SPI_HALF_US);
}

static void lcd_spi_exec(const uint16_t* seq)
{
    for(; *seq != ST_END; ++seq) {
        if(*seq & 0x8000)
            mdelay(*seq & 0x7fff);
        else
            lcd_spi_write9(*seq & 0x1ff);
    }
}

/* Target hooks */

/* Only the panel timing is target data; the rest is in dpu-x1600.c */
const struct lcd_tgt_config lcd_tgt_config = {
    .pixclock       = R1_PIXCLOCK_HZ,
    .hsync_len      = R1_HSYNC_LEN,
    .left_margin    = R1_LEFT_MARGIN,
    .right_margin   = R1_RIGHT_MARGIN,
    .vsync_len      = R1_VSYNC_LEN,
    .upper_margin   = R1_UPPER_MARGIN,
    .lower_margin   = R1_LOWER_MARGIN,
};

void lcd_tgt_enable(bool enable)
{
    if(enable) {
        /* Reset sequence, transcribed from the vendor module */
        gpio_set_level(GPIO_LCD_SPI_CS, 1);
        gpio_set_level(GPIO_LCD_SPI_SCK, 1);
        mdelay(10);

        gpio_set_level(GPIO_LCD_RST, 1);
        mdelay(5);
        gpio_set_level(GPIO_LCD_RST, 0);
        mdelay(50);
        gpio_set_level(GPIO_LCD_RST, 1);
        mdelay(30);

        /* Bring the pixel clock up before the panel expects data */
        lcd_set_clock(R1_LCD_CLKSRC, R1_PIXCLOCK_HZ);

        lcd_spi_exec(st7701s_init_seq);
    } else {
        /* Unverified; the vendor drops bldo1/bldo2 instead */
        gpio_set_level(GPIO_LCD_RST, 0);
    }
}

void lcd_tgt_sleep(bool sleep)
{
    if(sleep)
        lcd_spi_exec(st7701s_sleep_seq);
    else
        lcd_spi_exec(st7701s_wake_seq);
}
