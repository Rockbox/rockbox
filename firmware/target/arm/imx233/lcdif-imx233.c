/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2011 by Amaury Pouly
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
#include "lcdif-imx233.h"
#include "pinctrl-imx233.h"
#include "icoll-imx233.h"

#include "regs/lcdif.h"

#if IMX233_SUBTARGET >= 3700
static lcdif_irq_cb_t g_cur_frame_cb = NULL;
static lcdif_irq_cb_t g_vsync_edge_cb = NULL;
static lcdif_irq_cb_t g_underflow_cb = NULL;
#endif

/* for some crazy reason, all "non-dma" interrupts are routed to the ERROR irq */
#if IMX233_SUBTARGET >= 3700
void INT_LCDIF_ERROR(void)
{
    if(BF_RD(LCDIF_CTRL1, CUR_FRAME_DONE_IRQ))
    {
        if(g_cur_frame_cb)
            g_cur_frame_cb();
        BF_CLR(LCDIF_CTRL1, CUR_FRAME_DONE_IRQ);
    }
    if(BF_RD(LCDIF_CTRL1, VSYNC_EDGE_IRQ))
    {
        if(g_vsync_edge_cb)
            g_vsync_edge_cb();
        BF_CLR(LCDIF_CTRL1, VSYNC_EDGE_IRQ);
    }
    if(BF_RD(LCDIF_CTRL1, UNDERFLOW_IRQ))
    {
        if(g_underflow_cb)
            g_underflow_cb();
        BF_CLR(LCDIF_CTRL1, UNDERFLOW_IRQ);
    }
}
#endif

void imx233_lcdif_enable(bool enable)
{
    if(enable)
        BF_CLR(LCDIF_CTRL, CLKGATE);
    else
        BF_SET(LCDIF_CTRL, CLKGATE);
}

void imx233_lcdif_reset_lcd(bool enable)
{
#if IMX233_SUBTARGET < 3700
    if(enable)
        BF_SET(LCDIF_CTRL, RESET);
    else
        BF_CLR(LCDIF_CTRL, RESET);
#else
    if(enable)
        BF_SET(LCDIF_CTRL1, RESET);
    else
        BF_CLR(LCDIF_CTRL1, RESET);
#endif
}

void imx233_lcdif_init(void)
{
    imx233_reset_block(&HW_LCDIF_CTRL);
#if IMX233_SUBTARGET >= 3700
    imx233_icoll_enable_interrupt(INT_SRC_LCDIF_ERROR, true);
#endif
}

void imx233_lcdif_set_timings(unsigned data_setup, unsigned data_hold,
    unsigned cmd_setup, unsigned cmd_hold)
{
    BF_WR_ALL(LCDIF_TIMING, DATA_SETUP(data_setup),
        DATA_HOLD(data_hold), CMD_SETUP(cmd_setup), CMD_HOLD(cmd_hold));
}

void imx233_lcdif_set_word_length(unsigned word_length)
{
    switch(word_length)
    {
        case 8: BF_WR(LCDIF_CTRL, WORD_LENGTH_V(8_BIT)); break;
        case 16: BF_WR(LCDIF_CTRL, WORD_LENGTH_V(16_BIT)); break;
#if IMX233_SUBTARGET >= 3780
        case 18: BF_WR(LCDIF_CTRL, WORD_LENGTH_V(18_BIT)); break;
        case 24: BF_WR(LCDIF_CTRL, WORD_LENGTH_V(24_BIT)); break;
#endif
        default:
            panicf("this chip cannot handle a lcd word length of %d", word_length);
            break;
    }
}

void imx233_lcdif_wait_ready(void)
{
    while(BF_RD(LCDIF_CTRL, RUN));
}

void imx233_lcdif_set_data_swizzle(unsigned swizzle)
{
#if IMX233_SUBTARGET >= 3780
    BF_WR(LCDIF_CTRL, INPUT_DATA_SWIZZLE(swizzle));
#else
    BF_WR(LCDIF_CTRL, DATA_SWIZZLE(swizzle));
#endif
}

void imx233_lcdif_wait_fifo(void)
{
#if IMX233_SUBTARGET >= 3700
    while(BF_RD(LCDIF_STAT, TXFIFO_FULL));
#else
    while(!BF_RD(LCDIF_CTRL, FIFO_STATUS));
#endif
}

/* The following function set byte packing often, ifdefing everytime is painful */
#if IMX233_SUBTARGET < 3700
#define imx233_lcdif_set_byte_packing_format(a) 
#endif

// bbp = bytes per pixel
static void pio_send(unsigned len, unsigned bpp, uint8_t *buf)
{
    /* WARNING: the imx233 has a limitation on count wrt to byte packing, the
     * count must be a multiple of 2 with maximum packing when word-length is
     * 16-bit!
     * On the other hand, 8-bit word length doesn't seem to have any limitations,
     * for example one can send 3 bytes with a packing format of 0xf
     * WARNING for this function to work properly with any swizzle, we have to
     * make sure we pack as many 32-bits as possible even when the data is not
     * word-aligned */
    imx233_lcdif_set_byte_packing_format(0xf);
    /* compute shift between buf and next word-aligned pointer */
    int shift = 0;
    uint32_t temp_buf = 0;
    int count = len * bpp; // number of bytes
    while(0x3 & (intptr_t)buf)
    {
        temp_buf = temp_buf | *buf++ << shift;
        shift += 8;
        count--;
    }
    /* starting from now, all read are 32-bit */
    uint32_t *wbuf = (void *)buf;
#if IMX233_SUBTARGET >= 3780
    BF_WR_ALL(LCDIF_TRANSFER_COUNT, V_COUNT(1), H_COUNT(len));
#else
    BF_WR(LCDIF_CTRL, COUNT(len));
#endif
    BF_SET(LCDIF_CTRL, RUN);
    while(count > 0)
    {
        uint32_t val = *wbuf++;
        imx233_lcdif_wait_fifo();
        HW_LCDIF_DATA = temp_buf | val << shift;
        if(shift != 0)
            temp_buf = val >> (32 - shift);
        count -= 4;
    }
    /* send remaining bytes if any */
    if(shift != 0)
    {
        imx233_lcdif_wait_fifo();
        HW_LCDIF_DATA = temp_buf;
    }
    imx233_lcdif_wait_ready();
}

void imx233_lcdif_pio_send(bool data_mode, unsigned len, void *buf)
{
    imx233_lcdif_wait_ready();
    if(len == 0)
        return;
#if IMX233_SUBTARGET >= 3780
    imx233_lcdif_enable_bus_master(false);
#endif
    if(data_mode)
        BF_SET(LCDIF_CTRL, DATA_SELECT);
    else
        BF_CLR(LCDIF_CTRL, DATA_SELECT);

    switch(BF_RD(LCDIF_CTRL, WORD_LENGTH))
    {
        case BV_LCDIF_CTRL_WORD_LENGTH__8_BIT: pio_send(len, 1, buf); break;
        case BV_LCDIF_CTRL_WORD_LENGTH__16_BIT: pio_send(len, 2, buf); break;
#if IMX233_SUBTARGET >= 3780
        case BV_LCDIF_CTRL_WORD_LENGTH__18_BIT: pio_send(len, 4, buf); break;
#endif
        default: panicf("Don't know how to handle this word length");
    }
}

void imx233_lcdif_dma_send(void *buf, unsigned width, unsigned height)
{
#if IMX233_SUBTARGET >= 3780
    imx233_lcdif_enable_bus_master(true);
    HW_LCDIF_CUR_BUF = (uint32_t)buf;
    BF_WR_ALL(LCDIF_TRANSFER_COUNT, V_COUNT(height), H_COUNT(width));
    BF_SET(LCDIF_CTRL, DATA_SELECT);
    BF_SET(LCDIF_CTRL, RUN);
#else
    (void) buf;
    (void) width;
    (void) height;
    panicf("Unimplemented");
#endif
}

static void setup_data_pins(unsigned bus_width)
{
    imx233_pinctrl_setup_vpin(VPIN_LCD_D0, "lcd d0", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_LCD_D1, "lcd d1", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_LCD_D2, "lcd d2", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_LCD_D3, "lcd d3", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_LCD_D4, "lcd d4", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_LCD_D5, "lcd d5", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_LCD_D6, "lcd d6", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_LCD_D7, "lcd d7", PINCTRL_DRIVE_4mA, false);
    if(bus_width >= 16)
    {
        imx233_pinctrl_setup_vpin(VPIN_LCD_D8, "lcd d8", PINCTRL_DRIVE_4mA, false);
        imx233_pinctrl_setup_vpin(VPIN_LCD_D9, "lcd d9", PINCTRL_DRIVE_4mA, false);
        imx233_pinctrl_setup_vpin(VPIN_LCD_D10, "lcd d10", PINCTRL_DRIVE_4mA, false);
        imx233_pinctrl_setup_vpin(VPIN_LCD_D11, "lcd d11", PINCTRL_DRIVE_4mA, false);
        imx233_pinctrl_setup_vpin(VPIN_LCD_D12, "lcd d12", PINCTRL_DRIVE_4mA, false);
        imx233_pinctrl_setup_vpin(VPIN_LCD_D13, "lcd d13", PINCTRL_DRIVE_4mA, false);
        imx233_pinctrl_setup_vpin(VPIN_LCD_D14, "lcd d14", PINCTRL_DRIVE_4mA, false);
        imx233_pinctrl_setup_vpin(VPIN_LCD_D15, "lcd d15", PINCTRL_DRIVE_4mA, false);
    }
#if IMX233_SUBTARGET >= 3780
    if(bus_width >= 18)
    {
        imx233_pinctrl_setup_vpin(VPIN_LCD_D16, "lcd d16", PINCTRL_DRIVE_4mA, false);
        imx233_pinctrl_setup_vpin(VPIN_LCD_D17, "lcd d17", PINCTRL_DRIVE_4mA, false);
    }
#endif
}

void imx233_lcdif_setup_system_pins(unsigned bus_width)
{
    imx233_pinctrl_setup_vpin(VPIN_LCD_RESET, "lcd reset", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_LCD_RS, "lcd rs", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_LCD_WR, "lcd wr", PINCTRL_DRIVE_4mA, false);
#ifdef VPIN_LCD_RD
    imx233_pinctrl_setup_vpin(VPIN_LCD_RD, "lcd rd", PINCTRL_DRIVE_4mA, false);
#endif
    imx233_pinctrl_setup_vpin(VPIN_LCD_CS, "lcd cs", PINCTRL_DRIVE_4mA, false);

    setup_data_pins(bus_width);
}

#if IMX233_SUBTARGET >= 3700
void imx233_lcdif_setup_dotclk_pins(unsigned bus_width, bool have_enable)
{
    if(have_enable)
        imx233_pinctrl_setup_vpin(VPIN_LCD_ENABLE, "lcd enable", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_LCD_RESET, "lcd reset", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_LCD_HSYNC, "lcd hsync", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_LCD_VSYNC, "lcd vsync", PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_setup_vpin(VPIN_LCD_DOTCLK, "lcd dotclk", PINCTRL_DRIVE_4mA, false);

    setup_data_pins(bus_width);
}

void imx233_lcdif_set_byte_packing_format(unsigned byte_packing)
{
    BF_WR(LCDIF_CTRL1, BYTE_PACKING_FORMAT(byte_packing));
}
#endif

#if IMX233_SUBTARGET >= 3700 && IMX233_SUBTARGET < 3780
void imx233_lcdif_enable_sync_signals(bool en)
{
    BF_WR(LCDIF_VDCTRL3, SYNC_SIGNALS_ON(en));
}

void imx233_lcdif_setup_dotclk(unsigned v_pulse_width, unsigned v_period,
    unsigned v_wait_cnt, unsigned v_active, unsigned h_pulse_width,
    unsigned h_period, unsigned h_wait_cnt, unsigned h_active, bool enable_present)
{
    BF_WR_ALL(LCDIF_VDCTRL0, ENABLE_PRESENT(enable_present),
        VSYNC_PERIOD_UNIT(1), VSYNC_PULSE_WIDTH_UNIT(1),
         DOTCLK_V_VALID_DATA_CNT(v_active));
    BF_WR_ALL(LCDIF_VDCTRL1, VSYNC_PERIOD(v_period),
        VSYNC_PULSE_WIDTH(v_pulse_width));
    BF_WR_ALL(LCDIF_VDCTRL2, HSYNC_PULSE_WIDTH(h_pulse_width),
        HSYNC_PERIOD(h_period), DOTCLK_H_VALID_DATA_CNT(h_active));
    BF_WR_ALL(LCDIF_VDCTRL3, VERTICAL_WAIT_CNT(v_wait_cnt),
        HORIZONTAL_WAIT_CNT(h_wait_cnt));
    // setup dotclk mode, always bypass count, apparently data select is needed
    BF_SET(LCDIF_CTRL, DOTCLK_MODE, BYPASS_COUNT, DATA_SELECT);
}

void imx233_lcdif_setup_dotclk_ex(unsigned v_pulse_width, unsigned v_back_porch,
    unsigned v_front_porch, unsigned h_pulse_width, unsigned h_back_porch,
    unsigned h_front_porch, unsigned width, unsigned height, unsigned clk_per_pix,
    bool enable_present)
{
    unsigned h_active = clk_per_pix * width;
    unsigned h_period = h_active + h_back_porch + h_front_porch;
    unsigned v_active = height;
    unsigned v_period = v_active + v_back_porch + v_front_porch;
    imx233_lcdif_setup_dotclk(v_pulse_width, v_period, v_back_porch, v_active,
        h_pulse_width, h_period, h_back_porch, h_active, enable_present);
}

void imx233_lcdif_enable_frame_done_irq(bool en)
{
    if(en)
        BF_SET(LCDIF_CTRL1, CUR_FRAME_DONE_IRQ_EN);
    else
        BF_CLR(LCDIF_CTRL1, CUR_FRAME_DONE_IRQ_EN);
    BF_CLR(LCDIF_CTRL1, CUR_FRAME_DONE_IRQ);
}

void imx233_lcdif_set_frame_done_cb(lcdif_irq_cb_t cb)
{
    g_cur_frame_cb = cb;
}

void imx233_lcdif_enable_vsync_edge_irq(bool en)
{
    if(en)
        BF_SET(LCDIF_CTRL1, VSYNC_EDGE_IRQ_EN);
    else
        BF_CLR(LCDIF_CTRL1, VSYNC_EDGE_IRQ_EN);
    BF_CLR(LCDIF_CTRL1, VSYNC_EDGE_IRQ);
}

void imx233_lcdif_set_vsync_edge_cb(lcdif_irq_cb_t cb)
{
    g_vsync_edge_cb = cb;
}

void imx233_lcdif_enable_underflow_irq(bool en)
{
    if(en)
        BF_SET(LCDIF_CTRL1, UNDERFLOW_IRQ_EN);
    else
        BF_CLR(LCDIF_CTRL1, UNDERFLOW_IRQ_EN);
    BF_CLR(LCDIF_CTRL1, UNDERFLOW_IRQ);
}

void imx233_lcdif_set_underflow_cb(lcdif_irq_cb_t cb)
{
    g_underflow_cb = cb;
}
#endif

#if IMX233_SUBTARGET >= 3780
void imx233_lcdif_set_lcd_databus_width(unsigned width)
{
    switch(width)
    {
        case 8: BF_WR(LCDIF_CTRL, LCD_DATABUS_WIDTH_V(8_BIT)); break;
        case 16: BF_WR(LCDIF_CTRL, LCD_DATABUS_WIDTH_V(16_BIT)); break;
        case 18: BF_WR(LCDIF_CTRL, LCD_DATABUS_WIDTH_V(18_BIT)); break;
        case 24: BF_WR(LCDIF_CTRL, LCD_DATABUS_WIDTH_V(24_BIT)); break;
        default:
            panicf("this chip cannot handle a lcd bus width of %d", width);
            break;
    }
}

void imx233_lcdif_enable_underflow_recover(bool enable)
{
    if(enable)
        BF_SET(LCDIF_CTRL1, RECOVER_ON_UNDERFLOW);
    else
        BF_CLR(LCDIF_CTRL1, RECOVER_ON_UNDERFLOW);
}

void imx233_lcdif_enable_bus_master(bool enable)
{
    if(enable)
        BF_SET(LCDIF_CTRL, LCDIF_MASTER);
    else
        BF_CLR(LCDIF_CTRL, LCDIF_MASTER);
}
#endif
