/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Heikki Hannikainen
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
#include "system.h"
#include <stdbool.h>
#include "font.h"
#include "lcd.h"
#include "button.h"
#include "powermgmt.h"
#include "adc.h"
#include "debug-target.h"

bool dbg_ports(void)
{
    int adc_battery_voltage;
#ifndef HAVE_LCD_BITMAP
    int currval = 0;
    int button;
#else
    int adc_battery_level;

    lcd_setfont(FONT_SYSFIXED);
#endif
    lcd_clear_display();

    while(1)
    {
#ifdef HAVE_LCD_BITMAP
        lcd_putsf(0, 0, "PADR: %04x", (unsigned short)PADR);
        lcd_putsf(0, 1, "PBDR: %04x", (unsigned short)PBDR);

        lcd_putsf(0, 2, "AN0: %03x AN4: %03x", adc_read(0), adc_read(4));
        lcd_putsf(0, 3, "AN1: %03x AN5: %03x", adc_read(1), adc_read(5));
        lcd_putsf(0, 4, "AN2: %03x AN6: %03x", adc_read(2), adc_read(6));
        lcd_putsf(0, 5, "AN3: %03x AN7: %03x", adc_read(3), adc_read(7));

        battery_read_info(&adc_battery_voltage, &adc_battery_level);
        lcd_putsf(0, 6, "Batt: %d.%03dV %d%%  ", adc_battery_voltage / 1000,
                 adc_battery_voltage % 1000, adc_battery_level);

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
        {
            lcd_setfont(FONT_UI);
            return false;
        }
#else /* !HAVE_LCD_BITMAP */

       if (currval == 0) {
            lcd_putsf(0, 0, "PADR: %04x", (unsigned short)PADR);
        } else if (currval == 1) {
            lcd_putsf(0, 0, "PBDR: %04x", (unsigned short)PBDR);
        } else {
            int idx = currval - 2; /* idx < 7 */
            lcd_putsf(0, 0, "AN%d: %03x", idx, adc_read(idx));
        }

        battery_read_info(&adc_battery_voltage, NULL);
        lcd_putsf(0, 1, "Batt: %d.%03dV", adc_battery_voltage / 1000,
                 adc_battery_voltage % 1000);
        lcd_update();

        button = button_get_w_tmo(HZ/5);
        switch(button)
        {
            case BUTTON_STOP:
            return false;

        case BUTTON_LEFT:
            currval--;
            if(currval < 0)
                currval = 9;
            break;

        case BUTTON_RIGHT:
            currval++;
            if(currval > 9)
                currval = 0;
            break;
        }
#endif
    }
    return false;
}
