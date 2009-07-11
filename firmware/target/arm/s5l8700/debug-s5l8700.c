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
    char buf[50];
    int line;

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        line = 0;
        
        /* _DEBUG_PRINTF statements can be added here to show debug info */
        _DEBUG_PRINTF("__dbg_ports");

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
            break;
    }
    lcd_setfont(FONT_UI);
    return false;
}

