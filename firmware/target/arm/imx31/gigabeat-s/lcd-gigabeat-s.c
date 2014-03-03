/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Will Robertson
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

#ifdef BOOTLOADER
#include <string.h> /* memset */
#endif
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "lcd-target.h"
#include "backlight-target.h"
#include "spi-imx31.h"
#include "mc13783.h"

#define MAIN_LCD_IDMAC_CHANNEL 14

extern bool lcd_on; /* lcd-memframe.c */
static bool lcd_powered = true;

/* Settings shadow regs */
#ifdef HAVE_LCD_CONTRAST
static uint8_t reg0x0b = 0x2f;
#endif
#ifdef HAVE_LCD_INVERT
static uint8_t reg0x27 = 0x00;
#endif
#ifdef HAVE_LCD_FLIP
static uint8_t reg0x06 = 0x04;
#endif

#if 0
/* Initialization data from OF bootloader. Identical to Gigabeat F/X. */
static const unsigned char lcd_init_data[50] =
{
 /* Reg   Val */
    0x0f, 0x01,
    0x09, 0x06,
    0x16, 0xa6,
    0x1e, 0x49,
    0x1f, 0x26,
    0x0b, 0x2f, /* Set contrast 0-63 */
    0x0c, 0x2b,
    0x19, 0x5e,
    0x1a, 0x15,
    0x1b, 0x15,
    0x1d, 0x01,
    0x00, 0x03,
    0x01, 0x10,
    0x02, 0x0a,
    0x06, 0x04, /* Set the orientation 0x02=upside down, 0x04=normal */
    0x08, 0x2e,
    0x24, 0x12,
    0x25, 0x3f,
    0x26, 0x0b,
    0x27, 0x00, /* Invert display 0x00=normal, 0x10=inverted */
    0x28, 0x00,
    0x29, 0xf6,
    0x2a, 0x03,
    0x2b, 0x0a,
    0x04, 0x01, /* Display ON */
};
#endif

static const struct spi_node lcd_spi_node =
{
    /* Original firmware settings for LCD panel communication */
    CSPI3_NUM,                      /* CSPI module 3 */
    CSPI_CONREG_CHIP_SELECT_SS1 |   /* Chip select 1 */
    CSPI_CONREG_DRCTL_DONT_CARE |   /* Don't care about CSPI_RDY */
    CSPI_CONREG_DATA_RATE_DIV_16 |  /* Clock = IPG_CLK/16 = 4,125,000Hz. */
    CSPI_BITCOUNT(32-1) |           /* All 32 bits are to be transferred */
    CSPI_CONREG_SSPOL |             /* SS active high */
    CSPI_CONREG_PHA |               /* Phase 1 operation */
    CSPI_CONREG_POL |               /* Active low polarity */
    CSPI_CONREG_MODE,               /* Master mode */
    0,                              /* SPI clock - no wait states */
};

static void lcd_write_reg(unsigned reg, unsigned val)
{
    /* packet: |00|rr|01|vv| */
    uint32_t packet = ((reg & 0xff) << 16) | 0x0100 | (val & 0xff);

    struct spi_transfer_desc xfer;

    xfer.node = &lcd_spi_node;
    xfer.txbuf = &packet;
    xfer.rxbuf = NULL;
    xfer.count = 1;
    xfer.callback = NULL;
    xfer.next = NULL;

    if (spi_transfer(&xfer))
    {
        /* Just busy wait; the interface is not used very much */
        while (!spi_transfer_complete(&xfer));
    }
}

static void lcd_enable_interface(bool enable)
{
    spi_enable_node(&lcd_spi_node, enable);
}

static void lcd_set_power(bool powered)
{
    if (powered)
    {
        lcd_powered = false;
        lcd_write_reg(0x04, 0x00);
        lcd_enable_interface(false);
        bitclr32(&GPIO3_DR, (1 << 12));
        mc13783_clear(MC13783_REGULATOR_MODE1, MC13783_VCAMEN);
    }
    else
    {
        mc13783_set(MC13783_REGULATOR_MODE1, MC13783_VCAMEN);
        bitset32(&GPIO3_DR, (1 << 12));
        lcd_enable_interface(true);
        lcd_write_reg(0x04, 0x01);
        lcd_powered = true;
    }
}

static void lcd_sync_settings(void)
{
#ifdef HAVE_LCD_CONTRAST
    lcd_write_reg(0x0b, reg0x0b);
#endif
#ifdef HAVE_LCD_INVERT
    lcd_write_reg(0x27, reg0x27);
#endif
#ifdef HAVE_LCD_FLIP
    lcd_write_reg(0x06, reg0x06);
#endif
}

/* LCD init */
void INIT_ATTR lcd_init_device(void)
{
    /* Move the framebuffer */
#ifdef BOOTLOADER
    /* Only do this once to avoid flicker */
    memset(FRAME, 0x00, FRAME_SIZE);
#endif
    IPU_IPU_IMA_ADDR = ((0x1 << 16) | (MAIN_LCD_IDMAC_CHANNEL << 4)) + (1 << 3);
    IPU_IPU_IMA_DATA = FRAME_PHYS_ADDR;

    lcd_on = true;

    lcd_enable_interface(true);
    lcd_sync_settings();
}

void lcd_sleep(void)
{
    if (!lcd_powered)
        return;

    IPU_IDMAC_CHA_EN &= ~(1ul << MAIN_LCD_IDMAC_CHANNEL);
    lcd_enable(false);
    lcd_set_power(false);
    _backlight_lcd_sleep();
}

void lcd_enable(bool state)
{
    if (state == lcd_on)
        return;

    if (state)
    {
        if (!lcd_powered)
            lcd_set_power(true);
        IPU_IDMAC_CHA_EN |= 1ul << MAIN_LCD_IDMAC_CHANNEL;
        lcd_sync_settings();
        sleep(HZ/50);
        lcd_on = true;
        lcd_update();
        send_event(LCD_EVENT_ACTIVATION, NULL);
    }
    else
    {
        lcd_on = false;
    }
}

#ifdef HAVE_LCD_CONTRAST
void lcd_set_contrast(int val)
{
    reg0x0b = val & 0x3f;

    if (!lcd_on)
        return;

    lcd_write_reg(0x0b, reg0x0b);
}

int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}
#endif /* HAVE_LCD_CONTRAST */

#ifdef HAVE_LCD_INVERT
void lcd_set_invert_display(bool yesno)
{
    reg0x27 = yesno ? 0x10 : 0x00;

    if (!lcd_on)
        return;

    lcd_write_reg(0x27, reg0x27);
}
#endif /* HAVE_LCD_INVERT */

#ifdef HAVE_LCD_FLIP
void lcd_set_flip(bool yesno)
{
    reg0x06 = yesno ? 0x02 : 0x04;

    if (!lcd_on)
        return;

    lcd_write_reg(0x06, reg0x06);
}
#endif /* HAVE_LCD_FLIP */
