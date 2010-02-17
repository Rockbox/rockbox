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
#include "font.h"
#include "sprintf.h"

/*  Skeleton for adding target specific debug info to the debug menu
 */

#define _DEBUG_PRINTF(a,varargs...) do { \
        snprintf(buf, sizeof(buf), (a), ##varargs); lcd_puts(0,line++,buf); \
        } while(0)


bool __dbg_hw_info(void)
{
    char buf[50];
    int line;

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        while(1)
        {
        lcd_clear_display();
        line = 0;
        
        /* _DEBUG_PRINTF statements can be added here to show debug info */
        _DEBUG_PRINTF("__dbg_hw_info");

        lcd_update();
        int btn = button_get_w_tmo(HZ/10);
        if(btn == (DEBUG_CANCEL|BUTTON_REL))
            goto end;
        else if(btn == (BUTTON_PLAY|BUTTON_REL))
            break;
        }
    }

end:
    lcd_setfont(FONT_UI);
    return false;
}

bool __dbg_ports(void)
{
    char buf[32];
    int line;

    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        lcd_clear_display();
        line = 0;
        
        _DEBUG_PRINTF("GPIO  0: %08x",(unsigned int)PDAT0);
        _DEBUG_PRINTF("GPIO  1: %08x",(unsigned int)PDAT1);
        _DEBUG_PRINTF("GPIO  2: %08x",(unsigned int)PDAT2);
        _DEBUG_PRINTF("GPIO  3: %08x",(unsigned int)PDAT3);
        _DEBUG_PRINTF("GPIO  4: %08x",(unsigned int)PDAT4);
        _DEBUG_PRINTF("GPIO  5: %08x",(unsigned int)PDAT5);
        _DEBUG_PRINTF("GPIO  6: %08x",(unsigned int)PDAT6);
        _DEBUG_PRINTF("GPIO  7: %08x",(unsigned int)PDAT7);
        _DEBUG_PRINTF("GPIO 10: %08x",(unsigned int)PDAT10);
        _DEBUG_PRINTF("GPIO 11: %08x",(unsigned int)PDAT11);
        _DEBUG_PRINTF("GPIO 13: %08x",(unsigned int)PDAT13);
        _DEBUG_PRINTF("GPIO 14: %08x",(unsigned int)PDAT14);
        _DEBUG_PRINTF("5USEC  : %08x",(unsigned int)FIVE_USEC_TIMER);
        _DEBUG_PRINTF("USEC   : %08x",(unsigned int)USEC_TIMER);
        _DEBUG_PRINTF("USECREG: %08x",(unsigned int)(*(REG32_PTR_T)(0x3C700084)));

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
            break;
    }
    lcd_setfont(FONT_UI);
    return false;
}

