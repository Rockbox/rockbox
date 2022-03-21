/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2013 by Amaury Pouly
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
#include <sys/types.h> /* off_t */
#include <string.h>
#include "cpu.h"
#include "system.h"
#include "backlight-target.h"
#include "lcd.h"
#include "lcdif-imx233.h"
#include "clkctrl-imx233.h"
#include "pinctrl-imx233.h"
#include "logf.h"

#ifdef HAVE_LCD_ENABLE
static bool lcd_on;
#endif

static void lcd_write_reg(uint8_t reg, void *data, int data_len)
{
    imx233_lcdif_set_data_swizzle(0);
    imx233_lcdif_set_word_length(8);
    imx233_lcdif_pio_send(false, 1, &reg);
    imx233_lcdif_pio_send(true, data_len, data);
}

static void lcd_init_seq(void)
{
    // seems compatible with ILI9163
    lcd_write_reg(1, NULL, 0); // software reset
    lcd_write_reg(0x11, NULL, 0); // sleep out
    mdelay(150);
    /* format: (<cmd> <data size> <data0> <dataN>)+ */
    static uint8_t init_seq[] =
    {
    /*  cmd  sz  data... */
        0x26, 1, 4, /* Gamma Set */
        0xb1, 2, 9, 0xd, /* Frame Rate */
        0xc0, 2, 8, 0, /* Power Control 1 */
        0xc1, 1, 5, /* Power Control 2 */
        0xc5, 2, 0x31, 0x40, /* VCOM Control 1 */
        0xc7, 1, 0xc8, /* VCOM Offset Control */
        0xec, 1, 0xc, /* Unknown */
        0x3a, 1, 5, /* Interface Pixel Format */
        0x2a, 4, 0, 0, 0, 0x7f, /* Column Address */
        0x2b, 4, 0, 0, 0, 0x9f, /* Page Address Set */
        0x35, 1, 0, /* Tear Effect Line On */
        0x36, 1, 0xc8, /* Memory access Control */
        0xb4, 1, 0, /* Display Inversion */
        0xb7, 1, 0, /* Source Driver Direction Control */
        0xb8, 1, 0, /* Gate Driver Direction Control */
        0xf2, 1, 1, /* Gamma Adjustment */
        0xe0, 15, 0x3f, 0x20, 0x1d, 0x2d, 0x26, 0x0c, 0x4b, 0xb7,
                  0x39, 0x17, 0x1d, 0x16, 0x16, 0x10, 0x00, /* Positive Gamma */
        0xe1, 15, 0x00, 0x1f, 0x21, 0x12, 0x18, 0x13, 0x34, 0x48,
                  0x46, 0x08, 0x21, 0x29, 0x28, 0x2f, 0x3f, /* Negative Gamma */
        0x29, 0, /* Display On */
    };

    for(unsigned i = 0; i < sizeof(init_seq); )
    {
        lcd_write_reg(init_seq[i], &init_seq[i + 2], init_seq[i + 1]);
        i += 2 + init_seq[i + 1];
    }
}

void lcd_init_device(void)
{
    /* the LCD seems to work at 24Mhz, so use the xtal clock with no divider */
    imx233_clkctrl_enable(CLK_PIX, false);
    imx233_clkctrl_set_div(CLK_PIX, 1);
    imx233_clkctrl_set_bypass(CLK_PIX, true); /* use XTAL */
    imx233_clkctrl_enable(CLK_PIX, true);
    imx233_lcdif_init();
    imx233_lcdif_set_lcd_databus_width(8);
    imx233_lcdif_set_timings(2, 2, 2, 2);
    imx233_lcdif_enable_underflow_recover(true);
    imx233_lcdif_setup_system_pins(8);
    imx233_lcdif_set_byte_packing_format(0xf); /* two pixels per 32-bit word */

    // reset device
    imx233_lcdif_reset_lcd(true);
    mdelay(10);
    imx233_lcdif_reset_lcd(false);
    mdelay(10);
    imx233_lcdif_reset_lcd(true);
    mdelay(150);

    lcd_init_seq();
#ifdef HAVE_LCD_ENABLE
    lcd_on = true;
#endif
}

#ifdef HAVE_LCD_ENABLE
bool lcd_active(void)
{
    return lcd_on;
}

static void lcd_enable_seq(bool enable)
{
    if(enable)
    {
        lcd_write_reg(0x11, NULL, 0);
        lcd_write_reg(0x29, NULL, 0);
    }
    else
    {
        lcd_write_reg(0x28, NULL, 0);
        lcd_write_reg(0x10, NULL, 0);
    }
}

void lcd_enable(bool enable)
{
    if(lcd_on == enable)
        return;

    lcd_on = enable;

    if(enable)
        imx233_lcdif_enable(true);
    lcd_enable_seq(enable);
    if(!enable)
        imx233_lcdif_enable(false);
    else
        send_event(LCD_EVENT_ACTIVATION, NULL);
}
#endif

void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

void lcd_update_rect(int x, int y, int w, int h)
{
#ifdef HAVE_LCD_ENABLE
    if(!lcd_on)
        return;
#endif
    /* make sure the rectangle is included in the screen */
    x = MIN(x, LCD_WIDTH);
    y = MIN(y, LCD_HEIGHT);
    w = MIN(w, LCD_WIDTH - x);
    h = MIN(h, LCD_HEIGHT - y);

    imx233_lcdif_wait_ready();
    uint32_t window = x << 8 | (x + w - 1) << 24;
    lcd_write_reg(0x2a, &window, sizeof(window));
    window = y << 8 | (y + h - 1) << 24;
    lcd_write_reg(0x2b, &window, sizeof(window));
    lcd_write_reg(0x2c, NULL, 0);

    imx233_lcdif_wait_ready();
    imx233_lcdif_set_data_swizzle(3);
    imx233_lcdif_set_word_length(8);
    /* there are two cases here:
     * - either width = LCD_WIDTH and we can directly memcopy a part of lcd_framebuffer to FRAME
     *   and send it
     * - either width != LCD_WIDTH and we have to build a contiguous copy of the rectangular area
     *   into FRAME before sending it (which is slower and doesn't use the hardware)
     * In all cases, FRAME just acts as a temporary buffer.
     * NOTE It's more interesting to do a copy to FRAME in all cases since in system mode
     * the clock runs at 24MHz which provides barely 10MB/s bandwidth compared to >100MB/s
     * for memcopy operations
     */
    if(w == LCD_WIDTH)
    {
        memcpy((void *)FRAME, FBADDR(x,y), w * h * sizeof(fb_data));
    }
    else
    {
        void* (*fbaddr)(int x, int y) = FB_CURRENTVP_BUFFER->get_address_fn;
        for(int i = 0; i < h; i++)
            memcpy((fb_data *)FRAME + i * w, fbaddr(x,y + i), w * sizeof(fb_data));
    }
    /* WARNING The LCDIF has a limitation on the vertical count ! In 16-bit packed mode
     * (which we used, ie 16-bit per pixel, 2 pixels per 32-bit words), the v_count
     * field must be a multiple of 2. Furthermore, it seems the lcd controller doesn't
     * really like when both w and h are even, probably because the writes to the GRAM
     * are done on several words and the controller requires dummy writes.
     * The workaround is to always make sure that we send a number of pixels which is
     * a multiple of 4 so that both the lcdif and the controller are happy. If any
     * of w or h is odd, we will send a copy of the first pixels as dummy writes. We will
     * send at most 3 bytes. We then send (w * h + 3) / 4 x 4 bytes.
     */
    if(w % 2 == 1 || h % 2 == 1)
    {
        /* copy three pixel after the last one */
        for(int i = 0; i < 3; i++)
            *((fb_data *)FRAME + w * h + i) = *((fb_data *)FRAME + i);
        /* WARNING we need to update w and h to reflect the pixel count BUT it
         * has no relation to w * h (it can even be 2 * prime). Hopefully, w <= 240 and
         * h <= 320 so w * h <= 76800 and (w * h + 3) / 4 <= 38400 which fits into
         * a 16-bit integer (horizontal count). */
        h = (w * h + 3) / 4;
        w = 4;
    }
    imx233_lcdif_dma_send((void *)FRAME_PHYS_ADDR, w * 2, h);
}

