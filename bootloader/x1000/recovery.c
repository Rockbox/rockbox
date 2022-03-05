/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021-2022 Aidan MacDonald
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

#include "x1000bootloader.h"
#include "lcd.h"
#include "font.h"
#include "button.h"
#include "kernel.h"
#include <string.h>

enum {
    MENUITEM_HEADING,
    MENUITEM_ACTION,
};

struct menuitem {
    int type;
    const char* text;
    void(*action)(void);
};

/* Defines the recovery menu contents */
static const struct menuitem recovery_items[] = {
    {MENUITEM_HEADING,  "System",                   NULL},
    {MENUITEM_ACTION,       "Start Rockbox",        &boot_rockbox},
    {MENUITEM_ACTION,       "USB mode",             &usb_mode},
    {MENUITEM_ACTION,       "Shutdown",             &shutdown},
    {MENUITEM_ACTION,       "Reboot",               &reboot},
    {MENUITEM_HEADING,  "Bootloader",               NULL},
    {MENUITEM_ACTION,       "Install or update",    &bootloader_install},
    {MENUITEM_ACTION,       "Backup",               &bootloader_backup},
    {MENUITEM_ACTION,       "Restore",              &bootloader_restore},
};

static void put_help_line(int line, const char* str1, const char* str2)
{
    int width = LCD_WIDTH / SYSFONT_WIDTH;
    lcd_puts(0, line, str1);
    lcd_puts(width - strlen(str2), line, str2);
}

void recovery_menu(void)
{
    const int n_items = sizeof(recovery_items)/sizeof(struct menuitem);

    int selection = 0;
    while(recovery_items[selection].type != MENUITEM_ACTION)
        ++selection;

    while(1) {
        clearscreen();
        putcenter_y(0, "Rockbox recovery menu");

        int top_line = 2;

        /* draw the menu */
        for(int i = 0; i < n_items; ++i) {
            switch(recovery_items[i].type) {
            case MENUITEM_HEADING:
                lcd_putsf(0, top_line+i, "[%s]", recovery_items[i].text);
                break;

            case MENUITEM_ACTION:
                lcd_puts(3, top_line+i, recovery_items[i].text);
                break;

            default:
                break;
            }
        }

        /* draw the selection marker */
        lcd_puts(0, top_line+selection, "=>");

        /* draw the help text */
        int line = (LCD_HEIGHT - SYSFONT_HEIGHT)/SYSFONT_HEIGHT - 3;
        put_help_line(line++, BL_DOWN_NAME "/" BL_UP_NAME, "move cursor");
        put_help_line(line++, BL_SELECT_NAME, "select item");
        put_help_line(line++, BL_QUIT_NAME, "power off");

        lcd_update();

        /* handle input */
        switch(get_button(TIMEOUT_BLOCK)) {
        case BL_SELECT: {
            if(recovery_items[selection].action)
                recovery_items[selection].action();
        } break;

        case BL_UP:
            for(int i = selection-1; i >= 0; --i) {
                if(recovery_items[i].action) {
                    selection = i;
                    break;
                }
            }
            break;

        case BL_DOWN:
            for(int i = selection+1; i < n_items; ++i) {
                if(recovery_items[i].action) {
                    selection = i;
                    break;
                }
            }
            break;

        case BL_QUIT:
            shutdown();
            break;

        default:
            break;
        }
    }
}
