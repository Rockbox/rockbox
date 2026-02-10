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
#include "clock-echoplayer.h"
#include "nvic-arm.h"
#include "spi-stm32h7.h"
#include "gpio-stm32h7.h"
#include "clock-stm32h7.h"
#include "regs/stm32h743/rcc.h"
#include "regs/stm32h743/spi.h"
#include "regs/stm32h743/ltdc.h"
#include <string.h>

#define MS_TO_TICKS(x) \
    (((x) + (1000 / HZ - 1)) / (1000 / HZ))

/* ILI9342C specifies 10 MHz max */
#define LCD_SPI_FREQ 10000000

#define ili_cmd(cmd, ...) \
    do { \
        uint16_t arr[] = {cmd, __VA_ARGS__}; \
        for (size_t i = 1; i < ARRAYLEN(arr); ++i) \
            arr[i] |= 0x100; \
        stm_spi_transmit(&spi, arr, sizeof(arr)); \
    } while (0)

struct stm_spi_config spi_cfg = {
    .instance = ITA_SPI5,
    .clock = &spi5_ker_clock,
    .freq = LCD_SPI_FREQ,
    .mode = STM_SPIMODE_HALF_DUPLEX,
    .proto = STM_SPIPROTO_MOTOROLA,
    .frame_bits = 9,
    .hw_cs_output = true,
};

static struct stm_spi spi;
static fb_data shadowfb[LCD_WIDTH*LCD_HEIGHT] IRAM_LCDFRAMEBUFFER;

enum lcd_controller_state
{
    RESET,  /* Controller in hardware reset, LTDC disabled */
    SLEEP,  /* Controller in sleep-in mode, LTDC disabled */
    AWAKE,  /* Controller in sleep-out mode, LTDC enabled */
};

static enum lcd_controller_state lcd_controller_state = RESET;

static void enable_ltdc(void)
{
    /* Enable LTDC clock */
    stm32_clock_enable(&ltdc_ker_clock);

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
    reg_var(LTDC_LAYER_CFBAR(0)) = (uintptr_t)&shadowfb[0];
    reg_assignf(LTDC_LAYER_CFBLR(0), CFBP(row_bytes), CFBLL(row_bytes + 7));
    reg_assignf(LTDC_LAYER_CFBLNR(0), CFBLNBR(LCD_HEIGHT));
    reg_assignf(LTDC_LAYER_CR(0), LEN(1));

    /* Reload shadow registers to enable layer 1 */
    reg_writef(LTDC_SRCR, IMR(1));

    /* Enable LTDC output */
    reg_writef(LTDC_GCR, LTDCEN(1));
}

static void disable_ltdc(void)
{
    reg_writef(LTDC_GCR, LTDCEN(0));

    stm32_clock_disable(&ltdc_ker_clock);
}

/* TODO: thread safety, is a mutex needed here? */

static void reset_lcd(void)
{
    if (lcd_controller_state != RESET)
    {
        if (lcd_controller_state == AWAKE)
            disable_ltdc();

        /* Must be >= 10 us to take effect */
        gpio_set_level(GPIO_LCD_RESET, 0);
        udelay(10);

        lcd_controller_state = RESET;
    }
}

static void sleep_lcd(void)
{
    if (lcd_controller_state == AWAKE)
    {
        /*
         * Send sleep in command -- empirically this seems to
         * require a delay of 10ms before disabling the LTDC.
         * The clock output appears to be necessary to fully
         * blank the screen before entering sleep mode.
         *
         * Sometimes there is an effect where the screen is
         * only partly blanked and only later fully blanked,
         * which goes away with a 20ms delay.
         */
        ili_cmd(0x10);
        sleep(MS_TO_TICKS(20));

        /* Disable LTDC */
        disable_ltdc();

        lcd_controller_state = SLEEP;
    }
}

static void wake_lcd(void)
{
    if (lcd_controller_state == RESET)
    {
        /* Release reset line */
        gpio_set_level(GPIO_LCD_RESET, 1);
        sleep(MS_TO_TICKS(5));

        /* Memory access control (X/Y invert, BGR panel) */
        ili_cmd(0x36, 0xc8);

        /* Pixel format set (18bpp for RGB bus, 16bpp for SPI bus) */
        ili_cmd(0x3a, 0x65);

        /* Send set EXTC command to allow configuring RGB interface */
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

        /* Display ON */
        ili_cmd(0x29);
    }

    if (lcd_controller_state != AWAKE)
    {
        /* Sync framebuffer & enable LTDC output */
        commit_dcache();
        enable_ltdc();

        /* Sleep out command */
        ili_cmd(0x11);
        sleep(MS_TO_TICKS(5));

        lcd_controller_state = AWAKE;
        send_event(LCD_EVENT_ACTIVATION, NULL);
    }
}

void lcd_init_device(void)
{
    /* Configure SPI bus */
    stm_spi_init(&spi, &spi_cfg);
    nvic_enable_irq(NVIC_IRQN_SPI5);

#ifndef BOOTLOADER
    /* Enable LTDC and LCD controller */
    wake_lcd();
#endif
}

bool lcd_active(void)
{
    return lcd_controller_state == AWAKE;
}

void lcd_enable(bool enable)
{
    if (enable)
        wake_lcd();
    else
        sleep_lcd();
}

void lcd_shutdown(void)
{
    reset_lcd();
}

void lcd_update(void)
{
    if (!lcd_active())
        return;

    /* TODO: optimize using DMA2D */
    memcpy(shadowfb, FBADDR(0, 0), sizeof(shadowfb));
    commit_dcache();
}

void lcd_update_rect(int x, int y, int width, int height)
{
    if (!lcd_active())
        return;

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

    /* TODO: optimize using DMA2D */
    for (int dy = 0; dy < height; ++dy)
    {
        fb_data *shaddr = &shadowfb[(y+dy)*LCD_WIDTH + x];
        memcpy(shaddr, FBADDR(x, y+dy), FB_DATA_SZ * width);
        commit_dcache_range(shaddr, FB_DATA_SZ * width);
    }
}

void spi5_irq_handler(void)
{
    stm_spi_irq_handler(&spi);
}
