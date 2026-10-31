/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2021 Aidan MacDonald, Dana Conrad
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
#include "lcd-erosqnative.h"
#include "power-erosqnative.h"
#include "kernel.h"
#include "lcd-x1000.h"
#include "gpio-ingenic.h"
#include "system.h"
#include "devicedata.h"

uint32_t eros_qn_lcd_id;
uint32_t eros_qn_lcd_id4;
uint32_t eros_qn_lcd_type;
int eros_qn_lcd_autodetected;

static void start_bitbang(void)
{
    gpioz_configure(GPIO_A, 0xFF, GPIOF_INPUT);
    gpio_set_function(GPIO_LCD_WR, GPIOF_OUTPUT(1));
    gpio_set_function(GPIO_LCD_DC, GPIOF_OUTPUT(1));
}

static void stop_bitbang(void)
{
    gpioz_configure(GPIO_A, 0xFF, GPIOF_DEVICE(1));
    gpio_set_function(GPIO_LCD_WR, GPIOF_DEVICE(1));
    gpio_set_function(GPIO_LCD_DC, GPIOF_DEVICE(1));
}

static void set_cs(int level)
{
    gpio_set_level(GPIO_LCD_CE, level);
}

static void set_dcx(int level)
{
    gpio_set_level(GPIO_LCD_DC, level);
}

static void set_wrx(int level)
{
    gpio_set_level(GPIO_LCD_WR, level);
}

static void set_rdx(int level)
{
    gpio_set_level(GPIO_LCD_RD, level);
}

static void set_data_out(uint8_t data)
{
    /* Assumes the pins are already in input or output mode */
    jz_set(GPIO_PAT0(GPIO_A), data & 0xFF);
    jz_clr(GPIO_PAT0(GPIO_A), (~data) & 0xFF);
    jz_clr(GPIO_PAT1(GPIO_A), 0xFF);
}

static void set_data_in(void)
{
    /* Assumes the pins are already in input or output mode */
    jz_set(GPIO_PAT1(GPIO_A), 0xFF);
}

static uint8_t read_data(void)
{
    return REG_GPIO_PIN(GPIO_A) & 0xFF;
}

static void send_command_byte(uint8_t cmd)
{
    set_dcx(0);
    set_wrx(0);
    set_data_out(cmd);
    udelay(1);
    set_wrx(1);
    set_dcx(1);
    set_data_in();
}

static uint8_t read_data_byte(void)
{
    set_rdx(0);
    udelay(1);
    uint8_t value = read_data();
    set_rdx(1);
    udelay(1);
    return value;
}

static uint32_t lcd_readid(void)
{
    uint32_t retval = 0;

    set_cs(0);
    udelay(1);

    send_command_byte(0x04);
    read_data_byte();
    retval |= read_data_byte() << 16;
    retval |= read_data_byte() << 8;
    retval |= read_data_byte();

    set_cs(1);
    udelay(1);

    return retval;
}

static uint32_t lcd_read_id4(void)
{
    uint32_t retval = 0;

    set_cs(0);
    udelay(1);

    send_command_byte(0xD3);
    read_data_byte();
    retval |= read_data_byte() << 16;
    retval |= read_data_byte() << 8;
    retval |= read_data_byte();

    set_cs(1);
    udelay(1);

    return retval;
}

/* for reference on these command/data hex values, see the mipi dcs lcd spec.  *
 * Not everything here is there, but all the standard stuff is.                */


/* New Display Eroq 2.1 / Hifiwalker 1.7+ / Surfans v3.2, unknown Controller     *
 * (likely GC9308 based on ID code, similar to GC9306 / GC9A01)                  *
 *   https://espruino.microcosm.app/api/v1/files/ \                              *
 *   9dc1b976d621a2ab3854312cce862c4a9a50dc1b.html#GC9A01 ,                      *
 *   https://www.buydisplay.com/download/ic/GC9A01A.pdf ,                        *
 *   https://lcddisplay.co/wp-content/uploads/2023/02/GC9A01.pdf                 *
 *   https://www.smartdisplaytech.com/wp-content/uploads/2023/03/GC9306_spec.pdf *
 * Init sequence From 'EROS Q （c口）_V2.1_20231209固件.zip'                      *
 * update.upt/.iso -> In 'uboot.bin' at 0x52da0-0x5305f                          *
 *  http://www.eroshifi.com/download/firmware/122.html                           */
static const uint32_t erosqnative_lcd_cmd_enable_gc9308[] = {

    /* Unlock EXTC? */
    LCD_INSTR_CMD,      0xfe, // Inter Register Enable1
    LCD_INSTR_CMD,      0xef, // Inter Register Enable2

    LCD_INSTR_CMD,      0x36, // Memory Access Control
/* Bit7 1:vertical flip   0:no vertical flip
   Bit6 1:horizontal flip 0:no horizontal flip
   Bit3 1:BGR 0:RGB */
    LCD_INSTR_DAT,      0x90,
    /* Pixel Format Set */
    LCD_INSTR_CMD,      0x3a,
    LCD_INSTR_DAT,      0x55, /* Rockbox uses 16pp, OF specified 18 bpp */

    LCD_INSTR_CMD,      0x84, // ?? (undocumented)
    LCD_INSTR_DAT,      0x04,
    LCD_INSTR_CMD,      0x86, // ??
    LCD_INSTR_DAT,      0xfb,
    LCD_INSTR_CMD,      0x87, // ??
    LCD_INSTR_DAT,      0x79,
    LCD_INSTR_CMD,      0x89, // ??
    LCD_INSTR_DAT,      0x0b,
    LCD_INSTR_CMD,      0x8a, // ??
    LCD_INSTR_DAT,      0x20,
    LCD_INSTR_CMD,      0x8b, // ??
    LCD_INSTR_DAT,      0x80,
    LCD_INSTR_CMD,      0x8d, // ??
    LCD_INSTR_DAT,      0x3b,
    LCD_INSTR_CMD,      0x8e, // ??
    LCD_INSTR_DAT,      0xcf,

    LCD_INSTR_CMD,      0xec, // Charge Pump Frequent Control
    LCD_INSTR_DAT,      0x33,
    LCD_INSTR_DAT,      0x02,
    LCD_INSTR_DAT,      0x4c,

    LCD_INSTR_CMD,      0x98, // ?? (undocumented)
    LCD_INSTR_DAT,      0x3e,
    LCD_INSTR_CMD,      0x9c, // ??
    LCD_INSTR_DAT,      0x4b,
    LCD_INSTR_CMD,      0x99, // ??
    LCD_INSTR_DAT,      0x3e,
    LCD_INSTR_CMD,      0x9d, // ??
    LCD_INSTR_DAT,      0x4b,
    LCD_INSTR_CMD,      0x9b, // ??
    LCD_INSTR_DAT,      0x55,

    LCD_INSTR_CMD,      0xe8, // Frame Rate
    LCD_INSTR_DAT,      0x11,
    LCD_INSTR_DAT,      0x00,

    LCD_INSTR_CMD,      0xff, // ?? (Adafruit & Co lib.  C:0xFF, D:0x60, D:0x01, D:0x04)
    LCD_INSTR_DAT,      0x62, // LCD_INSTR_DAT, 0x01, LCD_INSTR_DAT, 0x04,
    LCD_INSTR_CMD,      0xc3, // Vreg1a voltage Control
    LCD_INSTR_DAT,      0x20,
    LCD_INSTR_CMD,      0xc4, // Vreg1b voltage Control
    LCD_INSTR_DAT,      0x03,
    LCD_INSTR_CMD,      0xc9, // Vreg2a voltage Control
    LCD_INSTR_DAT,      0x2a,

    LCD_INSTR_CMD,      0xf0, // SET_GAMMA1
    LCD_INSTR_DAT,      0x4a,
    LCD_INSTR_DAT,      0x10,
    LCD_INSTR_DAT,      0x0a,
    LCD_INSTR_DAT,      0x0a,
    LCD_INSTR_DAT,      0x26,
    LCD_INSTR_DAT,      0x39,

    LCD_INSTR_CMD,      0xf2, // SET_GAMMA3
    LCD_INSTR_DAT,      0x4a,
    LCD_INSTR_DAT,      0x10,
    LCD_INSTR_DAT,      0x0a,
    LCD_INSTR_DAT,      0x0a,
    LCD_INSTR_DAT,      0x26,
    LCD_INSTR_DAT,      0x39,

    LCD_INSTR_CMD,      0xf1, // SET_GAMMA2
    LCD_INSTR_DAT,      0x50,
    LCD_INSTR_DAT,      0x8f,
    LCD_INSTR_DAT,      0xaf,
    LCD_INSTR_DAT,      0x3b,
    LCD_INSTR_DAT,      0x3f,
    LCD_INSTR_DAT,      0x7f,

    LCD_INSTR_CMD,      0xf3, // SET_GAMMA4
    LCD_INSTR_DAT,      0x50,
    LCD_INSTR_DAT,      0x8f,
    LCD_INSTR_DAT,      0xaf,
    LCD_INSTR_DAT,      0x3b,
    LCD_INSTR_DAT,      0x3f,
    LCD_INSTR_DAT,      0x7f,

    LCD_INSTR_CMD,      0xba, // TE Control
    LCD_INSTR_DAT,      0x0a,

#ifdef BOOTLOADER
    LCD_INSTR_CMD,      0x35, // Tearing Effect Line ON
    LCD_INSTR_DAT,      0x00,
#endif

    LCD_INSTR_CMD,      0x21, /* Invert */

    /* Lock EXTC? */
    LCD_INSTR_CMD,      0xfe, // Inter Register Enable1
    LCD_INSTR_CMD,      0xee,

    /* Exit Sleep */
    LCD_INSTR_CMD,      0x11,
    LCD_INSTR_UDELAY,   120000,
    /* Display On */
    LCD_INSTR_CMD,      0x29,
    LCD_INSTR_UDELAY,   20000,
    LCD_INSTR_END,
};

/*
 * Original Display / Hifiwalker -1.5 / Surfans -2.7
 * Likely an ILI9342C (landscape version of ILI9341)
 */
static const uint32_t erosqnative_lcd_cmd_enable_ili9342c[] = {
    /* Set EXTC? */
    LCD_INSTR_CMD,      0xc8,
    LCD_INSTR_DAT,      0xff,
    LCD_INSTR_DAT,      0x93,
    LCD_INSTR_DAT,      0x42,
    /* Set Address Mode */
    LCD_INSTR_CMD,      0x36,
    LCD_INSTR_DAT,      0xd8,
    /* Pixel Format Set */
    LCD_INSTR_CMD,      0x3a,
    //LCD_INSTR_DAT,      0x66, /* OF specified 18 bpp */
    LCD_INSTR_DAT,      0x05, /* RB seems to be happier dealing with 16 bits/pixel */
    /* Power Control 1? */
    LCD_INSTR_CMD,      0xc0,
    LCD_INSTR_DAT,      0x15,
    LCD_INSTR_DAT,      0x15,
    /* Power Control 2? */
    LCD_INSTR_CMD,      0xc1,
    LCD_INSTR_DAT,      0x01,
    /* VCOM? */
    LCD_INSTR_CMD,      0xc5,
    LCD_INSTR_DAT,      0xda,
    /* Frame Rate Control */
    LCD_INSTR_CMD,      0xb1,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      0x1b,
    /* Display Inversion Control */
    LCD_INSTR_CMD,      0xb4,
    LCD_INSTR_DAT,      0x02,
    /* Positive gamma correction? */
    LCD_INSTR_CMD,      0xe0,
    LCD_INSTR_DAT,      0x0f,
    LCD_INSTR_DAT,      0x13,
    LCD_INSTR_DAT,      0x17,
    LCD_INSTR_DAT,      0x04,
    LCD_INSTR_DAT,      0x13,
    LCD_INSTR_DAT,      0x07,
    LCD_INSTR_DAT,      0x40,
    LCD_INSTR_DAT,      0x39,
    LCD_INSTR_DAT,      0x4f,
    LCD_INSTR_DAT,      0x06,
    LCD_INSTR_DAT,      0x0d,
    LCD_INSTR_DAT,      0x0a,
    LCD_INSTR_DAT,      0x1f,
    LCD_INSTR_DAT,      0x22,
    LCD_INSTR_DAT,      0x00,
    /* Negative gamma correction? */
    LCD_INSTR_CMD,      0xe1,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      0x21,
    LCD_INSTR_DAT,      0x24,
    LCD_INSTR_DAT,      0x03,
    LCD_INSTR_DAT,      0x0f,
    LCD_INSTR_DAT,      0x05,
    LCD_INSTR_DAT,      0x38,
    LCD_INSTR_DAT,      0x32,
    LCD_INSTR_DAT,      0x49,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      0x09,
    LCD_INSTR_DAT,      0x08,
    LCD_INSTR_DAT,      0x32,
    LCD_INSTR_DAT,      0x35,
    LCD_INSTR_DAT,      0x0f,
    /* Exit Sleep */
    LCD_INSTR_CMD,      0x11,
    LCD_INSTR_UDELAY,   120000,
    /* Display On */
    LCD_INSTR_CMD,      0x29,
    LCD_INSTR_UDELAY,   20000,
    LCD_INSTR_END,
};

static const uint32_t erosqnative_lcd_of_compat_cmd[] = {
    /* Pixel Format Set */
    LCD_INSTR_CMD,      0x3a,
    LCD_INSTR_DAT,      0x66, /* 18 bpp */
    /* Exit Sleep */
    LCD_INSTR_CMD,      0x11,
    LCD_INSTR_UDELAY,   120000,
    /* Display On */
    LCD_INSTR_CMD,      0x29,
    LCD_INSTR_UDELAY,   20000,
    LCD_INSTR_END,
};

/* sleep and wake copied directly from m3k */
static const uint32_t erosqnative_lcd_cmd_sleep[] = {
    /* Display OFF */
    LCD_INSTR_CMD,      0x28,
    /* Sleep IN */
    LCD_INSTR_CMD,      0x10,
    LCD_INSTR_UDELAY,   5000,
    LCD_INSTR_END,
};

static const uint32_t erosqnative_lcd_cmd_wake[] = {
    /* Sleep OUT */
    LCD_INSTR_CMD,      0x11,
    LCD_INSTR_UDELAY,   5000,
    /* Display ON */
    LCD_INSTR_CMD,      0x29,
    LCD_INSTR_END,
};

/* As far as I can tell, this is a sequence of commands sent before each
 * DMA set. Original in OF was:
 *          {0x2c, 0x2c, 0x2c, 0x2c}
 * But this set from the m3k seems to work the same, and makes more sense
 * to me:
 *          {0x00, 0x00, 0x00, 0x2c}
 * This command is more than likely going to be the same
 * for any old mipi lcd on the market, maybe. I really don't think we need
 * to send "write_memory_start four times in a row. */
static const uint8_t __attribute__((aligned(64)))
    erosqnative_lcd_dma_wr_cmd[] = {0x2c, 0x2c, 0x2c, 0x2c};

const struct lcd_tgt_config lcd_tgt_config = {
    .bus_width = 8,
    .cmd_width = 8,
    .use_6800_mode = 0,
    .use_serial = 0,
    .clk_polarity = 0,
    .dc_polarity = 0,
    .wr_polarity = 1,
    .te_enable = 0, /* OF had TE enabled (1) */
    .te_polarity = 1,
    .te_narrow = 0,
    .dma_wr_cmd_buf = &erosqnative_lcd_dma_wr_cmd,
    .dma_wr_cmd_size = sizeof(erosqnative_lcd_dma_wr_cmd),
};

void lcd_tgt_enable(bool enable)
{
    if(enable)
    {
        /* TODO: remove this after more testing */
        int device_ver;
#ifdef BOOTLOADER
        device_ver = EROSQN_VER;
#else
        device_ver = device_data.hw_rev;
#endif

        /* HW1 through HW3 units use the AXP192 and have an LCD power pin */
        if (eros_qn_pmu_type == EROS_QN_PMU_AXP192 ||
            (eros_qn_pmu_type < 0 && device_ver <= 3))
        {
            gpio_set_function(GPIO_LCD_PWR_HW1, GPIOF_OUTPUT(1));
            mdelay(20);
        }

        gpio_set_level(GPIO_LCD_RESET, 1);
        mdelay(12);

        /* perform display detection on first power-on */
        if (eros_qn_lcd_type == EROS_QN_LCDTYPE_UNKNOWN)
        {
            start_bitbang();

            eros_qn_lcd_id = lcd_readid();
            eros_qn_lcd_id4 = lcd_read_id4(); /* informational only */
            switch (eros_qn_lcd_id)
            {
            case 0xE30000:
                eros_qn_lcd_type = EROS_QN_LCDTYPE_ILI9342C;
                eros_qn_lcd_autodetected = true;
                break;

            case 0x009308:
                eros_qn_lcd_type = EROS_QN_LCDTYPE_GC9308;
                eros_qn_lcd_autodetected = true;
                break;

            /* fall back to hw_rev based method */
            default:
                if (device_ver >= 3)
                    eros_qn_lcd_type = EROS_QN_LCDTYPE_GC9308;
                else
                    eros_qn_lcd_type = EROS_QN_LCDTYPE_ILI9342C;
                break;
            }

            stop_bitbang();
        }

        /* set the clock */
        lcd_set_clock(X1000_CLK_SCLK_A, 20000000);

        /* toggle chip select low (active) */
        gpio_set_level(GPIO_LCD_RD, 1);
        gpio_set_level(GPIO_LCD_CE, 1);
        mdelay(5);
        gpio_set_level(GPIO_LCD_CE, 0);

        switch (eros_qn_lcd_type)
        {
        case EROS_QN_LCDTYPE_ILI9342C:
            lcd_exec_commands(&erosqnative_lcd_cmd_enable_ili9342c[0]);
            break;

        case EROS_QN_LCDTYPE_GC9308:
            lcd_exec_commands(&erosqnative_lcd_cmd_enable_gc9308[0]);
            break;
        }
    } else {
        /* doesn't flash white if we don't do anything... */
#if 0
        lcd_exec_commands(&erosqnative_lcd_cmd_sleep[0]);

        mdelay(115); // copied from m3k

        gpio_set_level(GPIO_LCD_PWR, 0);
        gpio_set_level(GPIO_LCD_RESET, 0);
#endif
    }
}

void lcd_tgt_enable_of(bool enable)
{
    /* silence the unused parameter warning */
    if (enable)
    {}

    lcd_exec_commands(&erosqnative_lcd_of_compat_cmd[0]);
}

void lcd_tgt_sleep(bool sleep)
{
    if(sleep)
        lcd_exec_commands(&erosqnative_lcd_cmd_sleep[0]);
    else
        lcd_exec_commands(&erosqnative_lcd_cmd_wake[0]);
}
