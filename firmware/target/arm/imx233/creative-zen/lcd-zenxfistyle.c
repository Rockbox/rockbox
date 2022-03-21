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
static int lcd_model;

static inline uint32_t encode_16_to_18(uint32_t a)
{
    return ((a & 0xff) << 1) | (((a >> 8) & 0xff) << 10);
}

static void lcd_write_reg(uint32_t reg, uint32_t data)
{
    uint32_t old_reg = reg;
    /* get back to 18-bit word length */
    imx233_lcdif_set_word_length(18);
    reg = encode_16_to_18(reg);
    data = encode_16_to_18(data);
    
    imx233_lcdif_pio_send(false, 1, &reg);
    if(old_reg != 0x22)
        imx233_lcdif_pio_send(true, 1, &data);
}

static void lcd_init_seq(void)
{
    if(lcd_model == 0)
    {
        lcd_write_reg(0xe3, 0x3008);
        lcd_write_reg(0xe7, 0x12);
        lcd_write_reg(0xef, 0x1231);
        lcd_write_reg(0x01, 0x100);
        lcd_write_reg(0x02, 0x700);
        lcd_write_reg(0x03, 0x1028);
        lcd_write_reg(0x04, 0);
        lcd_write_reg(0x08, 0x207);
        lcd_write_reg(0x09, 0);
        lcd_write_reg(0x0a, 0);
        lcd_write_reg(0x0c, 0);
        lcd_write_reg(0x0d, 0);
        lcd_write_reg(0x0f, 0);
        lcd_write_reg(0x10, 0);
        lcd_write_reg(0x11, 7);
        lcd_write_reg(0x12, 0);
        lcd_write_reg(0x13, 0);
        mdelay(200);
        lcd_write_reg(0x10, 0x1490);
        lcd_write_reg(0x11, 0x227);
        mdelay(50);
        lcd_write_reg(0x12, 0x9c);
        mdelay(50);
        lcd_write_reg(0x13, 0xc00);
        lcd_write_reg(0x29, 5);
        lcd_write_reg(0x2b, 0xc);
        lcd_write_reg(0x20, 0xef);
        lcd_write_reg(0x21, 0);
        lcd_write_reg(0x30, 6);
        lcd_write_reg(0x31, 0x703);
        lcd_write_reg(0x32, 0x206);
        lcd_write_reg(0x35, 4);
        lcd_write_reg(0x36, 0x1a05);
        lcd_write_reg(0x37, 0x600);
        lcd_write_reg(0x38, 0x307);
        lcd_write_reg(0x39, 0x707);
        lcd_write_reg(0x3c, 0x400);
        lcd_write_reg(0x3d, 0x50f);
        lcd_write_reg(0x50, 0);
        lcd_write_reg(0x51, 0xef);
        lcd_write_reg(0x52, 0);
        lcd_write_reg(0x53, 0x13f);
        lcd_write_reg(0x60, 0xa700);
        lcd_write_reg(0x61, 1);
        lcd_write_reg(0x6a, 0);
        lcd_write_reg(0x80, 0);
        lcd_write_reg(0x81, 0);
        lcd_write_reg(0x82, 0);
        lcd_write_reg(0x83, 0);
        lcd_write_reg(0x84, 0);
        lcd_write_reg(0x85, 0);
        lcd_write_reg(0x90, 0x10);
        lcd_write_reg(0x92, 0x600);
        lcd_write_reg(0x07, 0x133);
    }
    else
    {
        lcd_write_reg(0x01, 0x100);
        lcd_write_reg(0x02, 0x700);
        lcd_write_reg(0x03, 0x1028);
        lcd_write_reg(0x04, 0);
        lcd_write_reg(0x08, 0x207);
        lcd_write_reg(0x09, 0);
        lcd_write_reg(0x0a, 0);
        lcd_write_reg(0x0c, 0);
        lcd_write_reg(0x0d, 0);
        lcd_write_reg(0x0f, 0);
        lcd_write_reg(0x10, 0);
        lcd_write_reg(0x11, 7);
        lcd_write_reg(0x12, 0);
        lcd_write_reg(0x13, 0);
        mdelay(200);
        lcd_write_reg(0x10, 0x1290);
        lcd_write_reg(0x11, 0x227);
        mdelay(50);
        lcd_write_reg(0x12, 0x9c);
        mdelay(50);
        lcd_write_reg(0x13, 0x1f00);
        lcd_write_reg(0x29, 0x30);
        lcd_write_reg(0x2b, 0xd);
        lcd_write_reg(0x20, 0xef);
        lcd_write_reg(0x21, 0);
        lcd_write_reg(0x30, 0x404);
        lcd_write_reg(0x31, 0x404);
        lcd_write_reg(0x32, 0x404);
        lcd_write_reg(0x37, 0x303);
        lcd_write_reg(0x38, 0x303);
        lcd_write_reg(0x39, 0x303);
        lcd_write_reg(0x35, 0x103);
        lcd_write_reg(0x3c, 0x301);
        lcd_write_reg(0x36, 0x1e00);
        lcd_write_reg(0x3d, 0xf);
        lcd_write_reg(0x50, 0);
        lcd_write_reg(0x51, 0xef);
        lcd_write_reg(0x52, 0);
        lcd_write_reg(0x53, 0x13f);
        lcd_write_reg(0x60, 0xa700);
        lcd_write_reg(0x61, 0);
        lcd_write_reg(0x6a, 0);
        lcd_write_reg(0x80, 0);
        lcd_write_reg(0x81, 0);
        lcd_write_reg(0x82, 0);
        lcd_write_reg(0x83, 0);
        lcd_write_reg(0x84, 0);
        lcd_write_reg(0x85, 0);
        lcd_write_reg(0x2b, 0xd);
        mdelay(50);
        lcd_write_reg(0x90, 0x17);
        lcd_write_reg(0x92, 0);
        lcd_write_reg(0x93, 3);
        lcd_write_reg(0x95, 0x110);
        lcd_write_reg(0x97, 0);
        lcd_write_reg(0x98, 0);
        lcd_write_reg(0x07, 0x133);
    }
}

void lcd_init_device(void)
{
    /* clock at 24MHZ */
    imx233_clkctrl_enable(CLK_PIX, false);
    imx233_clkctrl_set_div(CLK_PIX, 1);
    imx233_clkctrl_set_bypass(CLK_PIX, true); /* use XTAL */
    imx233_clkctrl_enable(CLK_PIX, true);
    imx233_lcdif_init();
    imx233_lcdif_set_lcd_databus_width(18);
    imx233_lcdif_enable_underflow_recover(true);
    imx233_lcdif_setup_system_pins(18);
    imx233_lcdif_set_timings(2, 2, 2, 2);

    imx233_pinctrl_acquire(2, 8, "lcd_model");
    imx233_pinctrl_set_function(2, 8, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(2, 8, false);
    lcd_model = imx233_pinctrl_get_gpio(2, 8);

    // reset device
    imx233_lcdif_reset_lcd(true);
    mdelay(50);
    imx233_lcdif_reset_lcd(false);
    mdelay(10);
    imx233_lcdif_reset_lcd(true);

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
    if(!enable)
    {
    }
    else
    {
    }
}

void lcd_enable(bool enable)
{
    if(lcd_on == enable)
        return;

    lcd_on = enable;

    lcd_enable_seq(enable);
    if(enable)
        send_event(LCD_EVENT_ACTIVATION, NULL);
}
#endif

void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

/* NOTE the LCD is rotated: it's a 240x320 panel with (0,0) being the bottom-left
 * corner when the player is hold in landscape mode. This means x and y axis and
 * exchanged and the y axis is reversed. */

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
    lcd_write_reg(0x50, LCD_HEIGHT - y - h);
    lcd_write_reg(0x51, LCD_HEIGHT - y - 1);
    lcd_write_reg(0x52, x);
    lcd_write_reg(0x53, x + w - 1);
    lcd_write_reg(0x20, LCD_HEIGHT - y - 1);
    lcd_write_reg(0x21, x);
    lcd_write_reg(0x22, 0);
    imx233_lcdif_wait_ready();
    imx233_lcdif_set_word_length(16);
    imx233_lcdif_set_byte_packing_format(0xf); /* two pixels per 32-bit word */

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
    imx233_lcdif_dma_send((void *)FRAME_PHYS_ADDR, w, h);
}
