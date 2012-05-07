/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Rob Purchase
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

#include <stdbool.h>
#include <stdio.h>
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "string.h"
#include "button.h"
#include "lcd.h"
#include "font.h"
#include "adc.h"

/* IRQ status registers of debug interest only */
#define STS    (*(volatile unsigned long *)0xF3001008)
#define SRC    (*(volatile unsigned long *)0xF3001010)

bool dbg_ports(void)
{
    return false;
}

bool dbg_hw_info(void)
{
    int line = 0, i, oldline;

    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();

    /* Put all the static text before the while loop */
    lcd_puts(0, line++, "[Hardware info]");

    line++;
    oldline=line;
    
    while (1)
    {
        line = oldline;

        if (button_get_w_tmo(HZ/20) == (BUTTON_POWER|BUTTON_REL))
            break;

        lcd_putsf(0, line++, "current tick: %08lx Seconds running: %08ld",
            current_tick, current_tick/HZ);
            
        lcd_putsf(0, line++, "GPIOA: 0x%08lx  GPIOB: 0x%08lx", GPIOA, GPIOB);
        lcd_putsf(0, line++, "GPIOC: 0x%08lx  GPIOD: 0x%08lx", GPIOC, GPIOD);
        lcd_putsf(0, line++, "GPIOE: 0x%08lx",                 GPIOE);

        for (i = 0; i<4; i++)
            lcd_putsf(0, line++, "ADC%d: 0x%04x", i, adc_read(i));
        
        lcd_putsf(0, line++, "STS: 0x%08lx  SRC: 0x%08lx", STS, SRC);
        
        lcd_update();
    }
    return false;
}
