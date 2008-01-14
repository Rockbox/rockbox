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

bool __dbg_ports(void);
bool __dbg_ports(void)
{
    return false;
}

//extern char r_buffer[5];
//extern int r_button;

bool __dbg_hw_info(void);
bool __dbg_hw_info(void)
{
    int line = 0, button, oldline;
    int *address=0x0;
    bool done=false;
    char buf[100];

    lcd_setmargins(0, 0);
    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();

    /* Put all the static text before the while loop */
    lcd_puts(0, line++, "[Hardware info]");

    /* TODO: ... */

    line++;
    oldline=line;
    while(!done)
    {
        line = oldline;
        button = button_get(false);
        
        button &= ~BUTTON_REPEAT;
        
        if (button == BUTTON_MENU)
            done=true;
        if(button==BUTTON_DOWN)
            address+=0x01;
        else if (button==BUTTON_UP)
            address-=0x01;

        /*snprintf(buf, sizeof(buf), "Buffer: 0x%02x%02x%02x%02x%02x",
            r_buffer[0], r_buffer[1], r_buffer[2], r_buffer[3],r_buffer[4] );    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "Button: 0x%08x, HWread: 0x%08x",
            (unsigned int)button, r_button);  lcd_puts(0, line++, buf);*/
        snprintf(buf, sizeof(buf), "current tick: %08x Seconds running: %08d",
            (unsigned int)current_tick, (unsigned int)current_tick/100);  lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "Address: 0x%08x Data: 0x%08x", 
            (unsigned int)address, *address);           lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "Address: 0x%08x Data: 0x%08x",
            (unsigned int)(address+1), *(address+1));   lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "Address: 0x%08x Data: 0x%08x",
            (unsigned int)(address+2), *(address+2));   lcd_puts(0, line++, buf);

        lcd_update();
    }
    return false;
}
