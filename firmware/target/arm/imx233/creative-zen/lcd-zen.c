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
#include "kernel.h"
#include "backlight-target.h"
#include "lcd.h"
#include "lcdif-imx233.h"
#include "clkctrl-imx233.h"
#include "pinctrl-imx233.h"
#include "dma-imx233.h"
#include "logf.h"
#include "lcd-target.h"
#ifndef BOOTLOADER
#include "button.h"
#include "font.h"
#include "action.h"
#endif

#include "regs/lcdif.h"

/**
 * DMA
 */

/* Used for DMA */
struct lcdif_dma_command_t
{
    struct apb_dma_command_t dma;
    uint32_t ctrl;
} __attribute__((packed)) CACHEALIGN_ATTR;

__ENSURE_STRUCT_CACHE_FRIENDLY(struct lcdif_dma_command_t)

#define NR_CMDS ((IMX233_FRAMEBUFFER_SIZE + IMX233_MAX_SINGLE_DMA_XFER_SIZE - 1) / IMX233_MAX_SINGLE_DMA_XFER_SIZE)

struct lcdif_dma_command_t lcdif_dma[NR_CMDS];

/**
 * Utils
 */
static int g_wait_nr_frame = 0;
static struct semaphore g_wait_sema;

static void wait_frames_cb(void)
{
    if(--g_wait_nr_frame == 0)
        semaphore_release(&g_wait_sema);
}

static void wait_nr_frames(int nr)
{
    g_wait_nr_frame = 2 + nr; // +1 because we want entire frames, +1 to be safe
    imx233_lcdif_set_vsync_edge_cb(wait_frames_cb);
    imx233_lcdif_enable_vsync_edge_irq(true);
    semaphore_wait(&g_wait_sema, TIMEOUT_BLOCK);
    imx233_lcdif_enable_vsync_edge_irq(false);
}

/**
 * SPI
 */

#define SPI_CS(v) imx233_pinctrl_set_gpio(1, 11, v)
#define SPI_SCL(v) imx233_pinctrl_set_gpio(1, 10, v)
#define SPI_SDO(v) imx233_pinctrl_set_gpio(1, 9, v)

#define DEV_ID  0x74
#define RS      0x2
#define RW      0x1

static void spi_enable(bool en)
{
    imx233_pinctrl_set_gpio(1, 9, en);
    imx233_pinctrl_set_gpio(1, 10, en);
    imx233_pinctrl_set_gpio(1, 11, en);
    imx233_pinctrl_enable_gpio(1, 9, en);
    imx233_pinctrl_enable_gpio(1, 10, en);
    imx233_pinctrl_enable_gpio(1, 11, en);
    mdelay(1);
}

static void spi_delay(void)
{
    udelay(1);
}

static void spi_begin(void)
{
    SPI_CS(false);
    spi_delay();
}

static void spi_write(unsigned char b)
{
    for(int i = 7; i >= 0; i--)
    {
        SPI_SCL(false);
        spi_delay();
        SPI_SDO((b >> i) & 1);
        spi_delay();
        SPI_SCL(true);
        spi_delay();
    }
}

static void spi_end(void)
{
    SPI_CS(true);
    spi_delay();
}

static void spi_write_reg(uint8_t reg, uint16_t value)
{
    spi_begin();
    spi_write(DEV_ID);
    spi_write(0);
    spi_write(reg);
    spi_end();
    spi_begin();
    spi_write(DEV_ID | RS);
    spi_write(value >> 8);
    spi_write(value & 0xff);
    spi_end();
}

/**
 * LCD control
 */

static void lcd_something(bool en)
{
    /* I don't know what this pin does */
    imx233_pinctrl_set_gpio(1, 8, en);
    mdelay(10);
}

static void lcd_something_seq(void)
{
    spi_write_reg(0x7, 0);
    mdelay(10);
    spi_write_reg(0x12, 0x1618);
    spi_write_reg(0x11, 0x2227);
    spi_write_reg(0x13, 0x61d1);
    spi_write_reg(0x10, 0x550c);
    wait_nr_frames(5);
    spi_write_reg(0x12, 0x0c58);
}

static void lcd_init_seq(void)
{
    spi_write_reg(0x1, 0x231d);// no BGR inversion (OF uses BGR)
    spi_write_reg(0x2, 0x300);
    /* NOTE by default stmp3700 has vsync/hsync active low and data launch
     * at negative edge of dotclk, reflect this in the polarity settings */
    spi_write_reg(0x3, 0xd040);// polarity (OF uses 0xc040, seems incorrect)
    spi_write_reg(0x8, 0); // vsync back porch (0=3H)
    spi_write_reg(0x9, 0); // hsync back porch (0=24clk)
    spi_write_reg(0x76, 0x2213);
    spi_write_reg(0xb, 0x33e1);
    spi_write_reg(0xc, 0x23);
    spi_write_reg(0x76, 0);
    spi_write_reg(0xd, 7);
    spi_write_reg(0xe, 0);
    spi_write_reg(0x15, 0x803);
    spi_write_reg(0x14, 0);
    spi_write_reg(0x16, 0);
    spi_write_reg(0x30, 0x706);
    spi_write_reg(0x31, 0x406);
    spi_write_reg(0x32, 0xc09);
    spi_write_reg(0x33, 0x606);
    spi_write_reg(0x34, 0x706);
    spi_write_reg(0x35, 0x406);
    spi_write_reg(0x36, 0xc06);
    spi_write_reg(0x37, 0x601);
    spi_write_reg(0x38, 0x504);
    spi_write_reg(0x39, 0x504);
}

static void lcd_display_on_seq(void)
{
    spi_write_reg(0x7, 1);
    wait_nr_frames(1);
    spi_write_reg(0x7, 0x101);
    wait_nr_frames(2);
    spi_write_reg(0x76, 0x2213);
    spi_write_reg(0x1c, 0x6650);
    spi_write_reg(0xb, 0x33e1);
    spi_write_reg(0x76, 0);
    spi_write_reg(0x7, 0x103);
}

static void lcd_display_off_seq(void)
{
    spi_write_reg(0xb, 0x30e1);
    spi_write_reg(0x7, 0x102);
    wait_nr_frames(2);
    spi_write_reg(0x7, 0);
    spi_write_reg(0x12, 0);
    spi_write_reg(0x10, 0x100);
}

/**
 * Rockbox
 */

void lcd_enable(bool enable)
{
    if(lcd_active() == enable)
        return;

    lcd_set_active(enable);
    if(lcd_active())
    {
        // enable spi
        spi_enable(true);
        // reset
        imx233_lcdif_reset_lcd(true);
        imx233_lcdif_reset_lcd(false);
        mdelay(1);
        imx233_lcdif_reset_lcd(true);
        mdelay(1);
        // "power" on
        lcd_something(true);
        // setup registers
        lcd_something_seq();
        lcd_init_seq();
        lcd_display_on_seq();

        imx233_dma_reset_channel(APB_LCDIF);
        imx233_dma_start_command(APB_LCDIF, &lcdif_dma[0].dma);
    }
    else
    {
        // power down
        lcd_display_off_seq();
        lcd_something(false);
        // stop lcdif
        BF_CLR(LCDIF_CTRL, DOTCLK_MODE);
        /* stmp37xx errata: clearing DOTCLK_MODE won't clear RUN: wait until
         * fifo is empty and then clear manually */
        while(!BF_RD(LCDIF_STAT, TXFIFO_EMPTY));
        BF_CLR(LCDIF_CTRL, RUN);
        // disable spi
        spi_enable(false);
    }
}

static void lcd_underflow(void)
{
    /* on underflow, current frame is dead so stop lcdif and prepare for next frame
     * don't bother with the errata, fifo is empty since we are underflowing ! */
    BF_CLR(LCDIF_CTRL, DOTCLK_MODE);
    imx233_dma_reset_channel(APB_LCDIF);
    imx233_dma_start_command(APB_LCDIF, &lcdif_dma[0].dma);
}

void lcd_init_device(void)
{
    semaphore_init(&g_wait_sema, 1, 0);
    /* I'm not really sure this pin is related to power, it does not seem to do anything */
    imx233_pinctrl_acquire(1, 8, "lcd_something");
    imx233_pinctrl_acquire(1, 9, "lcd_spi_sdo");
    imx233_pinctrl_acquire(1, 10, "lcd_spi_scl");
    imx233_pinctrl_acquire(1, 11, "lcd_spi_cs");
    imx233_pinctrl_set_function(1, 9, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_set_function(1, 10, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_set_function(1, 11, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_set_function(1, 8, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(1, 8, true);
    /** lcd is 320x240, data bus is 8-bit, depth is 24-bit so we need 3clk/pix
     * by running PIX clock at 24MHz we can sustain ~100 fps */
    imx233_clkctrl_enable(CLK_PIX, false);
    imx233_clkctrl_set_div(CLK_PIX, 2);
    imx233_clkctrl_set_bypass(CLK_PIX, true); /* use XTAL */
    imx233_clkctrl_enable(CLK_PIX, true);
    imx233_lcdif_init();
    imx233_lcdif_setup_dotclk_pins(8, false);
    imx233_lcdif_set_word_length(8);
    imx233_lcdif_set_underflow_cb(&lcd_underflow);
    imx233_lcdif_enable_underflow_irq(true);
    imx233_dma_clkgate_channel(APB_LCDIF, true);
    imx233_dma_reset_channel(APB_LCDIF);
    /** Datasheet states:
     * 257H >= VBP >= 3H, VBP > VLW, VFP >= 1H
     * 1533clk >= HBP >= 24clk, HBP > HLW, HFP >= 4clk
     * 
     * Take VLW=1H, VBP=3H, VFP=1H, HLW=8, HBP=24, HFP=4
     * Take 3clk/pix because we send 24-bit/pix with 8-bit data bus
     * Keep consistent with register setting in lcd_init_seq
     */
    imx233_lcdif_setup_dotclk_ex(/*v_pulse_width*/1, /*v_back_porch*/3,
        /*v_front_porch*/1, /*h_pulse_width*/8, /*h_back_porch*/24,
        /*h_front_porch*/4, LCD_WIDTH, LCD_HEIGHT, /*clk_per_pix*/3,
        /*enable_present*/false);
    imx233_lcdif_set_byte_packing_format(0xf);
    imx233_lcdif_enable_sync_signals(true); // we need frame signals during init
    // setup dma
    unsigned size = IMX233_FRAMEBUFFER_SIZE;
    uint8_t *frame_p = FRAME;
    for(int i = 0; i < NR_CMDS; i++)
    {
        unsigned xfer = MIN(IMX233_MAX_SINGLE_DMA_XFER_SIZE, size);
        lcdif_dma[i].dma.next = &lcdif_dma[(i + 1) % NR_CMDS].dma;
        lcdif_dma[i].dma.cmd = BF_OR(APB_CHx_CMD, CHAIN(1),
            COMMAND(BV_APB_CHx_CMD_COMMAND__READ), XFER_COUNT(xfer));
        lcdif_dma[i].dma.buffer =  frame_p;
        size -= xfer;
        frame_p += xfer;
    }
    // first transfer: enable run, dotclk and so on
    lcdif_dma[0].dma.cmd |= BF_OR(APB_CHx_CMD, CMDWORDS(1));
    lcdif_dma[0].ctrl = BF_OR(LCDIF_CTRL, BYPASS_COUNT(1), DOTCLK_MODE(1),
        RUN(1), WORD_LENGTH(1));
    // enable
    lcd_enable(true);
}
