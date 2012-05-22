/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2012 by Amaury Pouly
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
static unsigned lcd_yuv_options = 0;

static void setup_parameters(void)
{
    imx233_lcdif_reset();
    imx233_lcdif_set_lcd_databus_width(HW_LCDIF_CTRL__LCD_DATABUS_WIDTH_16_BIT);
    imx233_lcdif_set_word_length(HW_LCDIF_CTRL__WORD_LENGTH_16_BIT);
    imx233_lcdif_set_timings(2, 2, 3, 3);
}

static void setup_lcd_pins(bool use_lcdif)
{
    /* WARNING
     * the B1P22 pins is used to gate the speaker! Do NOT drive
     * them as lcd_dotclk and lcd_hsync or it will break audio */
    imx233_pinctrl_acquire_pin(1, 18, "lcd reset");
    imx233_pinctrl_acquire_pin(1, 19, "lcd rs");
    imx233_pinctrl_acquire_pin(1, 20, "lcd wr");
    imx233_pinctrl_acquire_pin(1, 21, "lcd cs");
    imx233_pinctrl_acquire_pin(1, 23, "lcd enable");
    imx233_pinctrl_acquire_pin(1, 25, "lcd vsync");
    imx233_pinctrl_acquire_pin_mask(1, 0x3ffff, "lcd data");
    if(use_lcdif)
    {
        imx233_set_pin_function(1, 25, PINCTRL_FUNCTION_MAIN); /* lcd_vsync */
        imx233_set_pin_function(1, 21, PINCTRL_FUNCTION_MAIN); /* lcd_cs */
        imx233_set_pin_function(1, 23, PINCTRL_FUNCTION_MAIN); /* lcd_enable */
        imx233_set_pin_function(1, 18, PINCTRL_FUNCTION_MAIN); /* lcd_reset */
        imx233_set_pin_function(1, 19, PINCTRL_FUNCTION_MAIN); /* lcd_rs */
        imx233_set_pin_function(1, 16, PINCTRL_FUNCTION_MAIN); /* lcd_d16 */
        imx233_set_pin_function(1, 17, PINCTRL_FUNCTION_MAIN); /* lcd_d17 */
        imx233_set_pin_function(1, 20, PINCTRL_FUNCTION_MAIN); /* lcd_wr */
        __REG_CLR(HW_PINCTRL_MUXSEL(2)) = 0xffffffff; /* lcd_d{0-15} */
    }
    else
    {
        __REG_SET(HW_PINCTRL_MUXSEL(2)) = 0xffffffff; /* lcd_d{0-15} */
        imx233_enable_gpio_output_mask(1, 0x2bfffff, false); /* lcd_{d{0-17},reset,rs,wr,cs,enable,vsync} */
        imx233_set_pin_function(1, 16, PINCTRL_FUNCTION_GPIO); /* lcd_d16 */
        imx233_set_pin_function(1, 17, PINCTRL_FUNCTION_GPIO); /* lcd_d17 */
        imx233_set_pin_function(1, 19, PINCTRL_FUNCTION_GPIO); /* lcd_rs */
        imx233_set_pin_function(1, 20, PINCTRL_FUNCTION_GPIO); /* lcd_wr */
        imx233_set_pin_function(1, 21, PINCTRL_FUNCTION_GPIO); /* lcd_cs */
        imx233_set_pin_function(1, 23, PINCTRL_FUNCTION_GPIO); /* lcd_enable */
        imx233_set_pin_function(1, 25, PINCTRL_FUNCTION_GPIO); /* lcd_vsync */
    }
}

static void common_lcd_enable(bool enable)
{
    imx233_lcdif_enable(enable);
    setup_lcd_pins(enable); /* use GPIO pins when disable */
}

static void setup_lcdif(void)
{
    setup_parameters();
    common_lcd_enable(true);
    imx233_lcdif_enable_bus_master(true);
}

static void setup_lcdif_clock(void)
{
    /* the LCD seems to work at 24Mhz, so use the xtal clock with no divider */
    imx233_clkctrl_enable_clock(CLK_PIX, false);
    imx233_clkctrl_set_clock_divisor(CLK_PIX, 1);
    imx233_clkctrl_set_bypass_pll(CLK_PIX, true); /* use XTAL */
    imx233_clkctrl_enable_clock(CLK_PIX, true);
}

static void lcd_write_reg(uint32_t reg, uint32_t data)
{
    imx233_lcdif_pio_send(false, 2, &reg);
    if(reg != 0x22)
        imx233_lcdif_pio_send(true, 2, &data);
}

#define REG_UDELAY  0xffffffff
struct lcd_sequence_entry_t
{
    uint32_t reg, data;
};

static void lcd_send_sequence(struct lcd_sequence_entry_t *seq, unsigned count)
{
    for(;count-- > 0; seq++)
    {
        if(seq->reg == REG_UDELAY)
            udelay(seq->data);
        else
            lcd_write_reg(seq->reg, seq->data);
    }
}

#define _begin_seq() static struct lcd_sequence_entry_t __seq[] = {
#define _udelay(a) {REG_UDELAY, a},
#define _lcd_write_reg(a, b) {a, b},
#define _end_seq() }; lcd_send_sequence(__seq, sizeof(__seq) / sizeof(__seq[0]));

static void lcd_init_seq(void)
{
    _begin_seq()
    _lcd_write_reg(1, 0x11c)
    _lcd_write_reg(2, 0x100)
    _lcd_write_reg(3, 0x1030)
    _lcd_write_reg(8, 0x808)
    _lcd_write_reg(0xc, 0)
    _lcd_write_reg(0xf, 0xc01)
    _lcd_write_reg(0x20, 0)
    _lcd_write_reg(0x21, 0)
    _udelay(30)
    _lcd_write_reg(0x10, 0xa00)
    _lcd_write_reg(0x11, 0x1038)
    _udelay(30)
    _lcd_write_reg(0x12, 0x1010)
    _lcd_write_reg(0x13, 0x50)
    _lcd_write_reg(0x14, 0x4f58)
    _lcd_write_reg(0x30, 0)
    _lcd_write_reg(0x31, 0xdb)
    _lcd_write_reg(0x32, 0)
    _lcd_write_reg(0x33, 0)
    _lcd_write_reg(0x34, 0xdb)
    _lcd_write_reg(0x35, 0)
    _lcd_write_reg(0x36, 0xaf)
    _lcd_write_reg(0x37, 0)
    _lcd_write_reg(0x38, 0xdb)
    _lcd_write_reg(0x39, 0)
    _lcd_write_reg(0x50, 0)
    _lcd_write_reg(0x51, 0x705)
    _lcd_write_reg(0x52, 0xe0a)
    _lcd_write_reg(0x53, 0x300)
    _lcd_write_reg(0x54, 0xa0e)
    _lcd_write_reg(0x55, 0x507)
    _lcd_write_reg(0x56, 0)
    _lcd_write_reg(0x57, 3)
    _lcd_write_reg(0x58, 0x90a)
    _lcd_write_reg(0x59, 0xa09)
    _udelay(30)
    _lcd_write_reg(7, 0x1017)
    _udelay(40)
    _end_seq()
}

void lcd_init_device(void)
{
    setup_lcdif();
    setup_lcdif_clock();

    // reset device
    __REG_SET(HW_LCDIF_CTRL1) = HW_LCDIF_CTRL1__RESET;
    mdelay(50);
    __REG_CLR(HW_LCDIF_CTRL1) = HW_LCDIF_CTRL1__RESET;
    mdelay(10);
    __REG_SET(HW_LCDIF_CTRL1) = HW_LCDIF_CTRL1__RESET;

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
        _begin_seq()
        _end_seq()
    }
    else
    {
        _begin_seq()
        _end_seq()
    }
}

void lcd_enable(bool enable)
{
    if(lcd_on == enable)
        return;

    lcd_on = enable;
    
    if(enable)
        common_lcd_enable(true);
    lcd_enable_seq(enable);
    if(!enable)
        common_lcd_enable(false);
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
    lcd_write_reg(0x37, x);
    lcd_write_reg(0x36, x + w - 1);
    lcd_write_reg(0x39, y);
    lcd_write_reg(0x38, y + h - 1);
    lcd_write_reg(0x20, x);
    lcd_write_reg(0x21, y);
    lcd_write_reg(0x22, 0);
    
    imx233_lcdif_wait_ready();
    imx233_lcdif_set_word_length(HW_LCDIF_CTRL__WORD_LENGTH_16_BIT);
    imx233_lcdif_set_byte_packing_format(0xf); /* two pixels per 32-bit word */
    imx233_lcdif_set_data_format(false, false, false); /* RGB565, don't care, don't care */

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
        for(int i = 0; i < h; i++)
            memcpy((fb_data *)FRAME + i * w, FBADDR(x,y + i), w * sizeof(fb_data));
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

void lcd_yuv_set_options(unsigned options)
{
    lcd_yuv_options = options;
}

#define YFAC    (74)
#define RVFAC   (101)
#define GUFAC   (-24)
#define GVFAC   (-51)
#define BUFAC   (128)

static inline int clamp(int val, int min, int max)
{
    if (val < min)
        val = min;
    else if (val > max)
        val = max;
    return val;
}

void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    const unsigned char *ysrc, *usrc, *vsrc;
    int linecounter;
    fb_data *dst, *row_end;
    long z;

    /* width and height must be >= 2 and an even number */
    width &= ~1;
    linecounter = height >> 1;

    #if LCD_WIDTH >= LCD_HEIGHT
    dst     = FBADDR(x,y);
    row_end = dst + width;
    #else
    dst     = FBADDR(LCD_WIDTH - y - 1,x);
    row_end = dst + LCD_WIDTH * width;
    #endif

    z    = stride * src_y;
    ysrc = src[0] + z + src_x;
    usrc = src[1] + (z >> 2) + (src_x >> 1);
    vsrc = src[2] + (usrc - src[1]);

    /* stride => amount to jump from end of last row to start of next */
    stride -= width;

    /* upsampling, YUV->RGB conversion and reduction to RGB565 in one go */

    do
    {
        do
        {
            int y, cb, cr, rv, guv, bu, r, g, b;

            y  = YFAC*(*ysrc++ - 16);
            cb = *usrc++ - 128;
            cr = *vsrc++ - 128;

            rv  =            RVFAC*cr;
            guv = GUFAC*cb + GVFAC*cr;
            bu  = BUFAC*cb;

            r = y + rv;
            g = y + guv;
            b = y + bu;

            if ((unsigned)(r | g | b) > 64*256-1)
            {
                r = clamp(r, 0, 64*256-1);
                g = clamp(g, 0, 64*256-1);
                b = clamp(b, 0, 64*256-1);
            }

            *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

            #if LCD_WIDTH >= LCD_HEIGHT
            dst++;
            #else
            dst += LCD_WIDTH;
            #endif

            y = YFAC*(*ysrc++ - 16);
            r = y + rv;
            g = y + guv;
            b = y + bu;

            if ((unsigned)(r | g | b) > 64*256-1)
            {
                r = clamp(r, 0, 64*256-1);
                g = clamp(g, 0, 64*256-1);
                b = clamp(b, 0, 64*256-1);
            }

            *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

            #if LCD_WIDTH >= LCD_HEIGHT
            dst++;
            #else
            dst += LCD_WIDTH;
            #endif
        }
        while (dst < row_end);

        ysrc    += stride;
        usrc    -= width >> 1;
        vsrc    -= width >> 1;

        #if LCD_WIDTH >= LCD_HEIGHT
        row_end += LCD_WIDTH;
        dst     += LCD_WIDTH - width;
        #else
        row_end -= 1;
        dst     -= LCD_WIDTH*width + 1;
        #endif

        do
        {
            int y, cb, cr, rv, guv, bu, r, g, b;

            y  = YFAC*(*ysrc++ - 16);
            cb = *usrc++ - 128;
            cr = *vsrc++ - 128;

            rv  =            RVFAC*cr;
            guv = GUFAC*cb + GVFAC*cr;
            bu  = BUFAC*cb;

            r = y + rv;
            g = y + guv;
            b = y + bu;

            if ((unsigned)(r | g | b) > 64*256-1)
            {
                r = clamp(r, 0, 64*256-1);
                g = clamp(g, 0, 64*256-1);
                b = clamp(b, 0, 64*256-1);
            }

            *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

            #if LCD_WIDTH >= LCD_HEIGHT
            dst++;
            #else
            dst += LCD_WIDTH;
            #endif

            y = YFAC*(*ysrc++ - 16);
            r = y + rv;
            g = y + guv;
            b = y + bu;

            if ((unsigned)(r | g | b) > 64*256-1)
            {
                r = clamp(r, 0, 64*256-1);
                g = clamp(g, 0, 64*256-1);
                b = clamp(b, 0, 64*256-1);
            }

            *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

            #if LCD_WIDTH >= LCD_HEIGHT
            dst++;
            #else
            dst += LCD_WIDTH;
            #endif
        }
        while (dst < row_end);

        ysrc    += stride;
        usrc    += stride >> 1;
        vsrc    += stride >> 1;

        #if LCD_WIDTH >= LCD_HEIGHT
        row_end += LCD_WIDTH;
        dst     += LCD_WIDTH - width;
        #else
        row_end -= 1;
        dst     -= LCD_WIDTH*width + 1;
        #endif
    }
    while (--linecounter > 0);

    #if LCD_WIDTH >= LCD_HEIGHT
    lcd_update_rect(x, y, width, height);
    #else
    lcd_update_rect(LCD_WIDTH - y - height, x, height, width);
    #endif
}
