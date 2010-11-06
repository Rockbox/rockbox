/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
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
#include "lcd-remote.h"

bool dbg_ports(void)
{
    unsigned int gpio_out;
    unsigned int gpio1_out;
    unsigned int gpio_read;
    unsigned int gpio1_read;
    unsigned int gpio_function;
    unsigned int gpio1_function;
    unsigned int gpio_enable;
    unsigned int gpio1_enable;
    int adc_battery_voltage, adc_battery_level;
    int adc_buttons, adc_remote;
    int line;

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        line = 0;
        gpio_read = GPIO_READ;
        gpio1_read = GPIO1_READ;
        gpio_out = GPIO_OUT;
        gpio1_out = GPIO1_OUT;
        gpio_function = GPIO_FUNCTION;
        gpio1_function = GPIO1_FUNCTION;
        gpio_enable = GPIO_ENABLE;
        gpio1_enable = GPIO1_ENABLE;

        lcd_putsf(0, line++, "GPIO_READ: %08x", gpio_read);
        lcd_putsf(0, line++, "GPIO_OUT:  %08x", gpio_out);
        lcd_putsf(0, line++, "GPIO_FUNC: %08x", gpio_function);
        lcd_putsf(0, line++, "GPIO_ENA:  %08x", gpio_enable);
        lcd_putsf(0, line++, "GPIO1_READ: %08x", gpio1_read);
        lcd_putsf(0, line++, "GPIO1_OUT:  %08x", gpio1_out);
        lcd_putsf(0, line++, "GPIO1_FUNC: %08x", gpio1_function);
        lcd_putsf(0, line++, "GPIO1_ENA:  %08x", gpio1_enable);

        adc_buttons = adc_read(ADC_BUTTONS);
        adc_remote  = adc_read(ADC_REMOTE);
        battery_read_info(&adc_battery_voltage, &adc_battery_level);
#if defined(IAUDIO_X5) ||  defined(IAUDIO_M5) || defined(IRIVER_H300_SERIES)
        lcd_putsf(0, line++, "ADC_BUTTONS (%c): %02x",
            button_scan_enabled() ? '+' : '-', adc_buttons);
#else
        lcd_putsf(0, line++, "ADC_BUTTONS: %02x", adc_buttons);
#endif
#if defined(IAUDIO_X5) || defined(IAUDIO_M5)
        lcd_putsf(0, line++, "ADC_REMOTE  (%c): %02x",
            remote_detect() ? '+' : '-', adc_remote);
#else
        lcd_putsf(0, line++, "ADC_REMOTE:  %02x", adc_remote);
#endif
#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
        lcd_putsf(0, line++, "ADC_REMOTEDETECT: %02x",
                 adc_read(ADC_REMOTEDETECT));
#endif

        battery_read_info(&adc_battery_voltage, &adc_battery_level);
        lcd_putsf(0, line++, "Batt: %d.%03dV %d%%  ", adc_battery_voltage / 1000,
                 adc_battery_voltage % 1000, adc_battery_level);

#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
        lcd_putsf(0, line++, "remotetype: %d", remote_type());
#endif

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
        {
            lcd_setfont(FONT_UI);
            return false;
        }
    }
    return false;
}
