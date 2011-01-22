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
#include <sys/types.h>
#include "inttypes.h"

#include "config.h"
#include "system.h"
#include "cpu.h"
#include "spi-imx31.h"
#include "mc13783.h"
#include "string.h"
#include "lcd.h"
#include "kernel.h"
#include "lcd-target.h"
#include "backlight-target.h"

#define MAIN_LCD_IDMAC_CHANNEL 14
#define LCDADDR(x, y) (&lcd_framebuffer[(y)][(x)])

/* Copies a rectangle from one framebuffer to another. Can be used in
   single transfer mode with width = num pixels, and height = 1 which
   allows a full-width rectangle to be copied more efficiently. */
extern void lcd_copy_buffer_rect(fb_data *dst, const fb_data *src,
                                 int width, int height);

static bool lcd_on = true;
static bool lcd_powered = true;
static unsigned lcd_yuv_options = 0;
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
    if (enable)
    {
        spi_enable_module(&lcd_spi_node);
    }
    else
    {
        spi_disable_module(&lcd_spi_node);
    }
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

    lcd_enable_interface(true);
    lcd_sync_settings();
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    fb_data *dst, *src;

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

bool lcd_active(void)
{
    return lcd_on;
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    if (!lcd_on)
        return;

    lcd_copy_buffer_rect((fb_data *)FRAME, &lcd_framebuffer[0][0],
                         LCD_WIDTH*LCD_HEIGHT, 1);
}

void lcd_yuv_set_options(unsigned options)
{
    lcd_yuv_options = options;
}

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_lines(fb_data *dst,
                                   unsigned char const * const src[3],
                                   int width,
                                   int stride);
extern void lcd_write_yuv420_lines_odither(fb_data *dst,
                                           unsigned char const * const src[3],
                                           int width,
                                           int stride,
                                           int x_screen, /* To align dither pattern */
                                           int y_screen);
/* Performance function to blit a YUV bitmap directly to the LCD */
/* For the Gigabeat - show it rotated */
/* So the LCD_WIDTH is now the height */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    /* Caches for chroma data so it only need be recaculated every other
       line */
    unsigned char const * yuv_src[3];
    off_t z;

    if (!lcd_on)
        return;

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;
    height >>= 1;

    y = LCD_WIDTH - 1 - y;
    fb_data *dst = (fb_data*)FRAME + x * LCD_WIDTH + y;

    z = stride*src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    if (lcd_yuv_options & LCD_YUV_DITHER)
    {
        do
        {
            lcd_write_yuv420_lines_odither(dst, yuv_src, width, stride, y, x);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            dst -= 2;
            y -= 2;
        }
        while (--height > 0);
    }
    else
    {
        do
        {
            lcd_write_yuv420_lines(dst, yuv_src, width, stride);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            dst -= 2;
        }
        while (--height > 0);
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
