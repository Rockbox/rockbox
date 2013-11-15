/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (c) 2013 by Lorenzo Miori
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
#include "dcp-imx233.h"
#include "logf.h"
#ifndef BOOTLOADER
#include "button.h"
#include "font.h"
#include "action.h"
#endif
#include "dma-imx233.h"

#include "regs/regs-lcdif.h"

/**
 * NOTE
 * We don't know exact LCD models nor we have datasheets for them
 * Register function are partly guessed from the values, others are guessed from other LCD
 * drivers and others have been confirmed studying their values
 */

static enum lcd_type_t
{
    LCD_TYPE_ZERO = 0,
    LCD_TYPE_ONE = 1
} lcd_type = LCD_TYPE_ZERO;

static void lcd_write_reg(uint16_t reg, uint16_t data)
{
    imx233_lcdif_pio_send(false, 1, &reg);
    if(reg != 0x22)
        imx233_lcdif_pio_send(true, 1, &data);
}

/*
 * The two LCD types require different initialization sequences
 */
void lcd_init_seq(void)
{
    switch (lcd_type)
    {
        case LCD_TYPE_ZERO:
        {
            lcd_write_reg(0x11, 0x1f1e);
            lcd_write_reg(0x38, 0xf0f);
            lcd_write_reg(0x12, 0x1101);
            lcd_write_reg(0x13, 0x808);
            lcd_write_reg(0x14, 0x3119);
            lcd_write_reg(0x10, 0x1a10);
            udelay(0xc350);
            lcd_write_reg(0x13, 0x83b);
            udelay(0x30d40);
            lcd_write_reg(1, 0x90c); /* Display mode */
            lcd_write_reg(2, 0x200);
            lcd_write_reg(3, 0x1030);
            lcd_write_reg(7, 5);
            lcd_write_reg(8, 0x503);
            lcd_write_reg(11, 0);
            lcd_write_reg(12, 0);
            /* Gamma control */
            lcd_write_reg(0x30, 0x606);
            lcd_write_reg(0x31, 0x606);
            lcd_write_reg(0x32, 0x305);
            lcd_write_reg(0x33, 2);
            lcd_write_reg(0x34, 0x503);
            lcd_write_reg(0x35, 0x606);
            lcd_write_reg(0x36, 0x606);
            lcd_write_reg(0x37, 0x200);

            lcd_write_reg(0x11, 0x1f1e);
            lcd_write_reg(0x38, 0xf0f);
            /* Set initial LCD limits and RAM settings */
            lcd_write_reg(0x40, 0); //BPP ?
            lcd_write_reg(0x42, 0x9f00);
            lcd_write_reg(0x43, 0);
            lcd_write_reg(0x44, 0x7f00); /* Horizontal initial refresh zone [0 - 127] */
            lcd_write_reg(0x45, 0x9f00); /* Vertical initial refresh zone [0 - 159] */

            lcd_write_reg(14, 0x13);
            lcd_write_reg(0xa9, 0x14);
            lcd_write_reg(0xa7, 0x30);
            lcd_write_reg(0xa8, 0x124);
            lcd_write_reg(0x6f, 0x1d00);
            lcd_write_reg(0x70, 3);
            lcd_write_reg(7, 1);
            lcd_write_reg(0x10, 0x1a10);
            udelay(0x9c40);
            lcd_write_reg(7, 0x21);
            lcd_write_reg(7, 0x23);
            udelay(0x9c40);
            lcd_write_reg(7, 0x37); /* Seems to be "power on" */
            break;
        }
        case LCD_TYPE_ONE:
        {
            lcd_write_reg(0, 1);
            udelay(0x2710);
            lcd_write_reg(0x11, 0x171b);
            lcd_write_reg(0x12, 0);
            lcd_write_reg(0x13, 0x80d);
            lcd_write_reg(0x14, 0x18);
            lcd_write_reg(0x10, 0x1a10);
            udelay(0xc350);
            lcd_write_reg(0x13, 0x81d);
            udelay(0xc350);
            lcd_write_reg(1, 0x90c); /* Display mode */
            lcd_write_reg(2, 0x200);
            lcd_write_reg(3, 0x1030);
            lcd_write_reg(7, 5);
            lcd_write_reg(8, 0x30a);
            lcd_write_reg(11, 4);
            lcd_write_reg(12, 0);
            /* Gamma control */
            lcd_write_reg(0x30, 0x300);
            lcd_write_reg(0x31, 0);
            lcd_write_reg(0x32, 0);
            lcd_write_reg(0x33, 0x404);
            lcd_write_reg(0x34, 0x707);
            lcd_write_reg(0x35, 0x700);
            lcd_write_reg(0x36, 0x703);
            lcd_write_reg(0x37, 4);

            lcd_write_reg(0x38, 0);
            /* Set initial LCD limits and RAM settings */
            lcd_write_reg(0x40, 0);
            lcd_write_reg(0x42, 0x9f00); /* LCD Display Start Address Register 0 */
            lcd_write_reg(0x43, 0); /* LCD Display Start Address Register 1 */
            lcd_write_reg(0x44, 0x7f00); /* Horizontal initial refresh zone [0 - 127] */
            lcd_write_reg(0x45, 0x9f00); /* Vertical initial refresh zone [0 - 159] */

            lcd_write_reg(7, 1);
            udelay(0x2710);
            lcd_write_reg(7, 0x21);
            lcd_write_reg(7, 0x23);
            udelay(0x2710);
            lcd_write_reg(7, 0x1037);
            udelay(0x2710);
            lcd_write_reg(7, 0x35);
            lcd_write_reg(7, 0x36);
            lcd_write_reg(7, 0x37);
            udelay(10000);
            break;
        }
        default:
            break;
    }
}

static void send_update_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    /* Set horizontal refresh zone */
    lcd_write_reg(0x44, (x | (y + w - 1) << 0x8));
    /* Set vertical refresh zone */
    lcd_write_reg(0x45, (y | (y + h - 1) << 0x8));
    lcd_write_reg(0x21, x | y << 8);
    /* Set register index to 0x22 to write screen data. 0 is mock value */
    lcd_write_reg(0x22, 0);
}

static void setup_lcd_pins(void)
{
    imx233_lcdif_setup_system_pins(16);
     /* lcd_rd */
    imx233_pinctrl_acquire(0, 9, "lcd rd");
    imx233_pinctrl_set_function(0, 9, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_set_gpio(0, 9, false);
    /*
     * This pin is important to know the LCD type
     * There are two types that require two different initialization sequences
     */
    /* lcd_tp */
    imx233_pinctrl_acquire(3, 12, "lcd type");
    imx233_pinctrl_set_function(3, 12, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(3, 12, false);
    /* Sense LCD Type */
    lcd_type = imx233_pinctrl_get_gpio(3, 12) ? LCD_TYPE_ONE : LCD_TYPE_ZERO;
}

static void setup_parameters(void)
{
    imx233_lcdif_init();
    imx233_lcdif_enable(true);
    imx233_lcdif_set_word_length(16);
    imx233_lcdif_set_data_swizzle(false);
    imx233_lcdif_set_timings(2, 1, 1, 1);
    BF_WR_V(LCDIF_CTRL, MODE86, 8080_MODE);

    imx233_lcdif_reset_lcd(true);
    udelay(50);
    imx233_lcdif_reset_lcd(false);
    udelay(10);
    imx233_lcdif_reset_lcd(true);
}

void lcd_init_device(void)
{
    /* Setup interface pins */
    setup_lcd_pins();
    /* Set LCD parameters */
    setup_parameters();
    /* Send initialization sequence to LCD */
    lcd_init_seq();
}

struct lcdif_cmd_t
{
    struct apb_dma_command_t dma;
    uint32_t ctrl0;
    uint32_t pad[4];
} __attribute__((packed)) CACHEALIGN_ATTR;

struct lcdif_cmd_t lcdif_dma;
void lcd_update(void)
{
    unsigned size = LCD_WIDTH * LCD_HEIGHT * sizeof(fb_data);

    send_update_rect(0,0,LCD_WIDTH,LCD_HEIGHT);
    /* We can safely do the transfer in a single shot, since 160 * 128 * 2 < 65k,
     * the maximum transfer size!
     */
    lcdif_dma.dma.cmd |= BF_OR3(APB_CHx_CMD, CMDWORDS(1), XFER_COUNT(size), COMMAND(2));
    lcdif_dma.ctrl0 = HW_LCDIF_CTRL & ~BM_LCDIF_CTRL_COUNT;
    lcdif_dma.ctrl0 |= BF_OR2(LCDIF_CTRL, COUNT(size/2), DATA_SELECT(1));
    lcdif_dma.dma.buffer = FBADDR(0,0);
    lcdif_dma.dma.cmd |= BM_APB_CHx_CMD_SEMAPHORE;

    imx233_dma_start_command(APB_LCDIF, &lcdif_dma.dma);
    imx233_dma_wait_completion(APB_LCDIF, HZ);
}

void lcd_update_rect(int x, int y, int w, int h)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    lcd_update();
}

#ifndef BOOTLOADER
bool lcd_debug_screen(void)
{
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        int button = get_action(CONTEXT_STD, HZ / 10);
        switch(button)
        {
            case ACTION_STD_NEXT:
            case ACTION_STD_PREV:
            case ACTION_STD_OK:
            case ACTION_STD_MENU:
                lcd_setfont(FONT_UI);
                return true;
            case ACTION_STD_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }

        lcd_clear_display();
        lcd_putsf(0, 0, "LCD type: %d", lcd_type);
        lcd_update();
        yield();
    }

    return true;
}
#endif
