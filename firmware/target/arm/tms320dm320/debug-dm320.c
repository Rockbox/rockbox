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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

bool __dbg_hw_info(void)
{
    int line = 0, button;
    int *address=0x0;
    bool done=false;
    char buf[100];
    
    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();
    lcd_puts(0, line++, "[Hardware info]");

    while(!done)
    {
        line = 0;
        button = button_get(false);
        button&=~BUTTON_REPEAT;
        if (button == BUTTON_POWER)
            done=true;
        if(button==BUTTON_RC_PLAY)
            address+=0x01;
        else if (button==BUTTON_RC_DOWN)
            address-=0x01;
        else if (button==BUTTON_RC_FF)
            address+=0x800;
        else if (button==BUTTON_RC_REW)
            address-=0x800;

        snprintf(buf, sizeof(buf), "current tick: %04x", (unsigned int)current_tick);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "Address: 0x%08x Data: 0x%08x", (unsigned int)address, *address);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "Address: 0x%08x Data: 0x%08x", (unsigned int)(address+1), *(address+1));
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "Address: 0x%08x Data: 0x%08x", (unsigned int)(address+2), *(address+2));
        lcd_puts(0, line++, buf);

        lcd_update();
    }
    return false;
}
