/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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
#include "button.h"
#include "lcd.h"
#include "x1000/cpm.h"
#include "x1000/gpio.h"

#ifdef FIIO_M3K
extern volatile int ft_last_x, ft_last_y;
extern volatile int ft_last_evt;
extern volatile long ft_last_int;
#endif

static void dbg_clocks(void)
{
    static const char* const clkname[] = {
        "APLL", "MPLL", "SCLK_A", "CPU", "L2CACHE",
        "AHB0", "AHB2", "PCLK", "LCD", "MSC0", "MSC1",
        "I2SMCLK", "I2SBCLK",
    };

    int line = 1;
    lcd_puts(0, line++, "CLOCK CONFIG");
    lcd_putsf(2, line++, "* CLKGR = %08x", REG_CPM_CLKGR);

    line++;
    lcd_puts(0, line++, "FREQUENCIES");
    for(int i = 0; i < X1000_CLK_COUNT; ++i) {
        unsigned hz = clk_get(i);
        unsigned khz = hz / 1000;
        unsigned mhz = khz / 1000;
        lcd_putsf(2, line++, "* %7s at %u.%03u MHz", clkname[i],
                  mhz, khz - (mhz * 1000));
    }

}

static void dbg_gpios(void)
{
    const char* const portname = "ABCD";

    int line = 1;
    lcd_puts(0, line++, "PIN STATE");
    for(int i = 0; i < 4; ++i)
        lcd_putsf(2, line++, "* GPIO %c: %08x", portname[i], REG_GPIO_PIN(i));

    line++;
    for(int i = 0; i < 4; ++i) {
        unsigned intr = REG_GPIO_INT(i);
        unsigned mask = REG_GPIO_MSK(i);
        unsigned pat0 = REG_GPIO_PAT0(i);
        unsigned pat1 = REG_GPIO_PAT1(i);
        lcd_putsf(0, line++, "PORT %c CONFIG", portname[i]);
        lcd_putsf(2, line++, "* INT  = %08x", intr);
        lcd_putsf(2, line++, "* MSK  = %08x", mask);
        lcd_putsf(2, line++, "* PAT0 = %08x", pat0);
        lcd_putsf(2, line++, "* PAT1 = %08x", pat1);
    }
}

#ifdef FIIO_M3K
static const char* get_ft_event_name(int type)
{
    switch(type) {
    case 0: return "press";
    case 1: return "release";
    case 2: return "motion";
    default: return "unknown";
    }
}
#endif

static void dbg_buttons(void)
{
    int line = 1;
    lcd_putsf(0, line++, "BUTTONS");
    lcd_putsf(2, line++, "* state = %08x", button_read_device());

#ifdef FIIO_M3K
    line++;
    lcd_putsf(0, line++, "TOUCHPAD");
    lcd_putsf(2, line++, "* %s event at (%d, %d)",
              get_ft_event_name(ft_last_evt), ft_last_x, ft_last_y);
    lcd_putsf(2, line++, "* %ld ticks ago", current_tick - ft_last_int);
#endif
}

static const struct {
    const char* name;
    void(*draw)(void);
} dbg_screens[] = {
    {"CLOCKS", dbg_clocks},
    {"GPIOs", dbg_gpios},
    {"BUTTONS", dbg_buttons},
    {0, 0},
};

#define DBG_BUTTON_EXIT BUTTON_POWER
#define DBG_BUTTON_NEXT BUTTON_RIGHT
#define DBG_BUTTON_PREV BUTTON_LEFT

static int dbg_get_button(void)
{
    int btn;
    do {
        btn = button_get_w_tmo(1);
    } while(btn & (BUTTON_REL|BUTTON_REPEAT));
    return btn;
}

bool dbg_hw_info(void)
{
    int screen = 0;

    lcd_set_foreground(LCD_WHITE);
    lcd_set_background(LCD_BLACK);

    while(true) {
        /* get user input */
        int btn = dbg_get_button();
        if(btn == DBG_BUTTON_EXIT)
            return false;
        else if(btn == DBG_BUTTON_NEXT)
            screen += 1;
        else if(btn == DBG_BUTTON_PREV)
            screen -= 1;

        /* keep screen index in bounds */
        if(screen < 0)
            screen = 0;
        else if(dbg_screens[screen].name == 0)
            screen -= 1;

        /* draw the screen */
        lcd_clear_display();
        dbg_screens[screen].draw();
        lcd_update();
        yield();
    }
}

bool dbg_ports(void)
{
    return false;
}
