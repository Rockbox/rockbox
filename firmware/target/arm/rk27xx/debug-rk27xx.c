/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
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
#include "config.h"
#include "kernel.h"
#include "debug-target.h"
#include "button.h"
#include "lcd.h"
#include "adc.h"
#include "font.h"
#include "storage.h"

/*  Skeleton for adding target specific debug info to the debug menu
 */

#define _DEBUG_PRINTF(a, varargs...) lcd_putsf(0, line++, (a), ##varargs);

extern unsigned long sd_debug_time_rd;
extern unsigned long sd_debug_time_wr;

bool dbg_hw_info(void)
{
    int line;
    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        lcd_clear_display();
        line = 0;

        /* _DEBUG_PRINTF statements can be added here to show debug info */
        _DEBUG_PRINTF("SCU_ID:      0x%0x", SCU_ID);
        _DEBUG_PRINTF("SCU_PLLCON1: 0x%0x", SCU_PLLCON1);
        _DEBUG_PRINTF("SCU_PLLCON2: 0x%0x", SCU_PLLCON2);
        _DEBUG_PRINTF("SCU_PLLCON3: 0x%0x", SCU_PLLCON3);
        _DEBUG_PRINTF("SCU_DIVCON1: 0x%0x", SCU_DIVCON1);
        _DEBUG_PRINTF("SCU_CLKCFG:  0x%0x", SCU_CLKCFG);
        _DEBUG_PRINTF("SCU_CHIPCFG: 0x%0x", SCU_CHIPCFG);
        line++;
        _DEBUG_PRINTF("sd_debug_time_rd: %d", sd_debug_time_rd);
        _DEBUG_PRINTF("sd_debug_time_wr: %d", sd_debug_time_wr);
        lcd_update(); 
        switch(button_get_w_tmo(HZ/20))
        {
            case DEBUG_CANCEL:
            case BUTTON_REL:
                lcd_setfont(FONT_UI);
                return false;
        }
    }

    lcd_setfont(FONT_UI);
    return false;
}

bool dbg_ports(void)
{
    int line;

    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        lcd_clear_display();
        line = 0;
        
        _DEBUG_PRINTF("GPIO_PADR:  %02x",(unsigned char)GPIO_PADR);
        _DEBUG_PRINTF("GPIO_PACON: %02x",(unsigned char)GPIO_PACON);
        _DEBUG_PRINTF("GPIO_PBDR:  %02x",(unsigned char)GPIO_PBDR);
        _DEBUG_PRINTF("GPIO_PBCON: %02x",(unsigned char)GPIO_PBCON);
        _DEBUG_PRINTF("GPIO_PCDR:  %02x",(unsigned char)GPIO_PCDR);
        _DEBUG_PRINTF("GPIO_PCCON: %02x",(unsigned char)GPIO_PCCON);
        _DEBUG_PRINTF("GPIO_PDDR:  %02x",(unsigned char)GPIO_PDDR);
        _DEBUG_PRINTF("GPIO_PDCON: %02x",(unsigned char)GPIO_PDCON);
        _DEBUG_PRINTF("ADC0: %d", adc_read(0));
        _DEBUG_PRINTF("ADC1: %d", adc_read(1));
        _DEBUG_PRINTF("ADC2: %d", adc_read(2));
        _DEBUG_PRINTF("ADC3: %d", adc_read(3));

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
            break;
    }
    lcd_setfont(FONT_UI);
    return false;
}

