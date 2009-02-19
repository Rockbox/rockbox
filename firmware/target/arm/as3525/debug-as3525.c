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
#include "debug-target.h"
#include "button.h"
#include "lcd.h"
#include "font.h"
#include "system.h"
#include "sprintf.h"
#include "cpu.h"

#define _DEBUG_PRINTF(a,varargs...) \
    snprintf(buf, sizeof(buf), (a), ##varargs); lcd_puts(0,line++,buf)

/* FIXME: target tree is including ./debug-target.h rather than the one in
 * sansa-fuze/, even though deps contains the correct one
 * if I put the below into a sansa-fuze/debug-target.h, it doesn't work*/
#if defined(SANSA_FUZE) || defined(SANSA_E200V2)
#define DEBUG_DBOP
short button_dbop_data(void);
#endif


/* TODO */

bool __dbg_hw_info(void)
{
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
        _DEBUG_PRINTF("[GPIO Values and Directions]");
        _DEBUG_PRINTF("GPIOA: %2x DIR: %2x", GPIOA_DATA, GPIOA_DIR);
        _DEBUG_PRINTF("GPIOB: %2x DIR: %2x", GPIOB_DATA, GPIOB_DIR);
        _DEBUG_PRINTF("GPIOC: %2x DIR: %2x", GPIOC_DATA, GPIOC_DIR);
        _DEBUG_PRINTF("GPIOD: %2x DIR: %2x", GPIOD_DATA, GPIOD_DIR);
#ifdef DEBUG_DBOP
        line++;
        _DEBUG_PRINTF("[DBOP_DIN]");
        _DEBUG_PRINTF("DBOP_DIN: %4x", button_dbop_data());
#endif
        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
            break;
    }
    lcd_setfont(FONT_UI);
    return false;
}
