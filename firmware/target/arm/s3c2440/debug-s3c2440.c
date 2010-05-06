/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
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
#include "font.h"
#include "debug-target.h"

bool __dbg_hw_info(void)
{
    return false;
}

bool __dbg_ports(void)
{
    int line;

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        line = 0;
        lcd_puts(0, line++, "[Ports and Registers]");

        lcd_putsf(0, line++, "GPACON: %08lx GPBCON: %08lx", GPACON, GPBCON);
        lcd_putsf(0, line++, "GPADAT: %08lx GPBDAT: %08lx", GPADAT, GPBDAT);
        lcd_putsf(0, line++, "GPAUP:  %08lx GPBUP:  %08lx", 0ul, GPBUP);
        lcd_putsf(0, line++, "GPCCON: %08lx GPDCON: %08lx", GPCCON, GPDCON);
        lcd_putsf(0, line++, "GPCDAT: %08lx GPDDAT: %08lx", GPCDAT, GPDDAT);
        lcd_putsf(0, line++, "GPCUP:  %08lx GPDUP:  %08lx", GPCUP, GPDUP);

        lcd_putsf(0, line++, "GPCCON: %08lx GPDCON: %08lx", GPCCON, GPDCON);
        lcd_putsf(0, line++, "GPCDAT: %08lx GPDDAT: %08lx", GPCDAT, GPDDAT);
        lcd_putsf(0, line++, "GPCUP:  %08lx GPDUP:  %08lx", GPCUP, GPDUP);

        lcd_putsf(0, line++, "GPECON: %08lx GPFCON: %08lx", GPECON, GPFCON);
        lcd_putsf(0, line++, "GPEDAT: %08lx GPFDAT: %08lx", GPEDAT, GPFDAT);
        lcd_putsf(0, line++, "GPEUP:  %08lx GPFUP:  %08lx", GPEUP, GPFUP);

        lcd_putsf(0, line++, "GPGCON: %08lx GPHCON: %08lx", GPGCON, GPHCON);
        lcd_putsf(0, line++, "GPGDAT: %08lx GPHDAT: %08lx", GPGDAT, GPHDAT);
        lcd_putsf(0, line++, "GPGUP:  %08lx GPHUP:  %08lx", GPGUP, GPHUP);

        lcd_putsf(0, line++, "GPJCON: %08lx", GPJCON);
        lcd_putsf(0, line++, "GPJDAT: %08lx", GPJDAT);
        lcd_putsf(0, line++, "GPJUP:  %08lx", GPJUP);
        
        line++;

        lcd_putsf(0, line++, "SRCPND:  %08lx INTMOD:  %08lx", SRCPND, INTMOD);
        lcd_putsf(0, line++, "INTMSK:  %08lx INTPND:  %08lx", INTMSK, INTPND);
        lcd_putsf(0, line++, "CLKCON:  %08lx CLKSLOW: %08lx", CLKCON, CLKSLOW);
        lcd_putsf(0, line++, "MPLLCON: %08lx UPLLCON: %08lx", MPLLCON, UPLLCON);
        lcd_putsf(0, line++, "CLKDIVN: %08lx CAMDIVN: %08lx", CLKDIVN, CAMDIVN);
        lcd_putsf(0, line++, "BWSCON:  %08lx TCONSEL: %08lx", BWSCON, TCONSEL);

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
            return false;
    }
}  
