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

#include "config.h"
#include "cpu.h"
#include "system.h"
#include "string.h"
#include <stdbool.h>
#include "button.h"
#include "lcd.h"
#include "sprintf.h"
#include "font.h"
#include "debug-target.h"
#include "adc.h"

bool __dbg_ports(void)
{
    return false;
}

bool __dbg_hw_info(void)
{
    int line = 0, i, button, oldline;
    bool done=false;
    char buf[100];

    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();

    /* Put all the static text before the while loop */
    lcd_puts(0, line++, "[Hardware info]");

    line++;
    oldline=line;
    while(!done)
    {
        line = oldline;
        button = button_get(false);
        
        button &= ~BUTTON_REPEAT;
        
        if (button == BUTTON_SELECT)
            done=true;

        snprintf(buf, sizeof(buf), "current tick: %08x Seconds running: %08d",
            (unsigned int)current_tick, (unsigned int)current_tick/100);  lcd_puts(0, line++, buf);
            
        snprintf(buf, sizeof(buf), "GPIOA: 0x%08x  GPIOB: 0x%08x",
            (unsigned int)GPIOA, (unsigned int)GPIOB);  lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIOC: 0x%08x  GPIOD: 0x%08x",
            (unsigned int)GPIOC, (unsigned int)GPIOD);  lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPIOE: 0x%08x",
            (unsigned int)GPIOE);  lcd_puts(0, line++, buf);

        for (i = 0; i<4; i++)
        {
            snprintf(buf, sizeof(buf), "ADC%d: 0x%04x", i, adc_read(i));
                lcd_puts(0, line++, buf);
        }
        
        lcd_update();
    }
    return false;
}
