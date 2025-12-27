/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "lcd-echoplayer.h"
#include "nvic-arm.h"
#include "spi-stm32h7.h"
#include "gpio-stm32h7.h"
#include "clock-stm32h7.h"
#include "regs/stm32h743/rcc.h"
#include "regs/stm32h743/spi.h"
#include "regs/stm32h743/ltdc.h"

/*
 * ILI9342C specifies 10 MHz max
 *
 * Use 12MHz for now -- for some reason using 6 MHz doesn't work
 * to enable RGB mode, but works fine to send graphics in SPI mode?
 */
#define LCD_SPI_FREQ 12000000

struct stm_spi_config spi_cfg = {
    .instance = ITA_SPI5,
    .clock = STM_CLOCK_SPI5_KER,
    .freq = LCD_SPI_FREQ,
    .mode = STM_SPIMODE_HALF_DUPLEX,
    .proto = STM_SPIPROTO_MOTOROLA,
    .frame_bits = 9,
    .hw_cs_output = true,
};

struct stm_spi spi;

#define ili_cmd(cmd, ...) \
    do { \
        uint16_t arr[] = {cmd, __VA_ARGS__}; \
        for (size_t i = 1; i < ARRAYLEN(arr); ++i) \
            arr[i] |= 0x100; \
        stm_spi_transmit(&spi, arr, sizeof(arr)); \
    } while (0)

static void init_ltdc(void)
{
    /* Enable LTDC clock */
    stm_clock_enable(STM_CLOCK_LTDC_KER);

    /* Set timing parameters */
    const uint32_t hsw  = LCD_HSW - 1;
    const uint32_t ahbp = hsw + LCD_HBP;
    const uint32_t aaw  = ahbp + LCD_HAW;
    const uint32_t totw = aaw + LCD_HFP;

    const uint32_t vsh  = LCD_VSH - 1;
    const uint32_t avbp = vsh + LCD_VBP;
    const uint32_t aah  = avbp + LCD_VAH;
    const uint32_t toth = aah + LCD_VFP;

    reg_writef(LTDC_SSCR, HSW(hsw), VSH(vsh));
    reg_writef(LTDC_BPCR, AHBP(ahbp), AVBP(avbp));
    reg_writef(LTDC_AWCR, AAW(aaw), AAH(aah));
    reg_writef(LTDC_TWCR, TOTALW(totw), TOTALH(toth));

    /* Set interface polarity */
    reg_writef(LTDC_GCR, HSPOL(0), VSPOL(0), DEPOL(0), PCPOL(0), DEN(0));

    /* Enable layer 1 to blit framebuffer to whole screen */
    const uint32_t row_bytes = LCD_WIDTH * FB_DATA_SZ;

    reg_assignf(LTDC_LAYER_WHPCR(0), WHSPPOS(ahbp + LCD_HAW), WHSTPOS(ahbp + 1));
    reg_assignf(LTDC_LAYER_WVPCR(0), WVSPPOS(avbp + LCD_VAH), WVSTPOS(avbp + 1));
    reg_assignf(LTDC_LAYER_PFCR(0), PF(BV_LTDC_LAYER_PFCR_PF_RGB565));
    reg_var(LTDC_LAYER_CFBAR(0)) = (uintptr_t)FBADDR(0, 0);
    reg_assignf(LTDC_LAYER_CFBLR(0), CFBP(row_bytes), CFBLL(row_bytes + 7));
    reg_assignf(LTDC_LAYER_CFBLNR(0), CFBLNBR(LCD_HEIGHT));
    reg_assignf(LTDC_LAYER_CR(0), LEN(1));

    /* Reload shadow registers to enable layer 1 */
    reg_writef(LTDC_SRCR, IMR(1));

    /* Enable LTDC output */
    reg_writef(LTDC_GCR, LTDCEN(1));
}

void lcd_init_device(void)
{
    /* Configure SPI bus */
    stm_spi_init(&spi, &spi_cfg);
    nvic_enable_irq(NVIC_IRQN_SPI5);

    /* Enable LCD controller */
    init_ltdc();

    /* Ensure controller is reset */
    gpio_set_level(GPIO_LCD_RESET, 0);
    sleep(12);

    gpio_set_level(GPIO_LCD_RESET, 1);
    sleep(12);

    /* Sleep out */
    ili_cmd(0x11);
    sleep(12);

    /* memory access control (X/Y invert, BGR panel) */
    ili_cmd(0x36, 0xc8);

    /* pixel format set (18bpp for RGB bus, 16bpp for SPI bus) */
    ili_cmd(0x3a, 0x65);

    /* send set EXTC command to allow configuring RGB interface */
    ili_cmd(0xc8, 0xff, 0x93, 0x42);

    /*
     * Enable RGB interface transferring to internal GRAM.
     *
     * Direct to shift register mode doesn't work; for one, the
     * framebuffer doesn't get transferred properly which might
     * just be timing issues. Two, the display is horizontally
     * flipped and there doesn't seem to be a way to change it
     * in the shift register mode.
     */
    ili_cmd(0xb0, 0xc0);
    ili_cmd(0xf6, 0x01, 0x00, 0x06);

    /* display ON */
    ili_cmd(0x29);
}

bool lcd_active(void)
{
    return true;
}

void lcd_update(void)
{
    commit_dcache();
}

void lcd_update_rect(int x, int y, int width, int height)
{
    if (x < 0)
        x = 0;
    else if (x >= LCD_WIDTH)
        x = LCD_WIDTH - 1;

    if (y < 0)
        y = 0;
    else if (y >= LCD_HEIGHT)
        y = LCD_HEIGHT - 1;

    if (width > LCD_WIDTH - x)
        width = LCD_WIDTH - x;

    if (height > LCD_HEIGHT - y)
        height = LCD_HEIGHT - y;

    for (int dy = y; dy < height; ++dy)
        commit_dcache_range(FBADDR(x, dy), FB_DATA_SZ * width);
}

void spi5_irq_handler(void)
{
    stm_spi_irq_handler(&spi);
}
