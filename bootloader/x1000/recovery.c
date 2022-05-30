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

static void to_main_menu(void);
static void to_debug_menu(void);

/* Defines the recovery menu contents */
static const struct menuitem recovery_items[] = {
    {MENUITEM_HEADING,  "Boot select",              NULL},
    {MENUITEM_ACTION,       "Rockbox",              &boot_rockbox},
#ifdef OF_PLAYER_NAME
    {MENUITEM_ACTION,       OF_PLAYER_NAME,         &boot_of_player},
#endif
#ifdef OF_RECOVERY_NAME
    {MENUITEM_ACTION,       OF_RECOVERY_NAME,       &boot_of_recovery},
#endif
    {MENUITEM_ACTION,       "Linux",                &boot_linux},
    {MENUITEM_HEADING,  "System",                   NULL},
    {MENUITEM_ACTION,       "USB mode",             &usb_mode},
    {MENUITEM_ACTION,       "Shutdown",             &shutdown},
    {MENUITEM_ACTION,       "Reboot",               &reboot},
    {MENUITEM_HEADING,  "Bootloader",               NULL},
    {MENUITEM_ACTION,       "Install or update",    &bootloader_install},
    {MENUITEM_ACTION,       "Backup",               &bootloader_backup},
    {MENUITEM_ACTION,       "Restore",              &bootloader_restore},
    {MENUITEM_HEADING,  "Advanced",                 NULL},
    {MENUITEM_ACTION,       "Debug tools",          &to_debug_menu},
};

static const struct menuitem debug_menu_items[] = {
    {MENUITEM_HEADING,  "Debug tools",              NULL},
#ifdef HAVE_SCREENDUMP
    {MENUITEM_ACTION,       "Enable screenshots",   &screenshot_enable},
#endif
    {MENUITEM_ACTION,       "Flash info",           &show_flash_info},
#ifdef OF_PLAYER_ADDR
    {MENUITEM_ACTION,       "Dump OF player",       &dump_of_player},
#endif
#ifdef OF_RECOVERY_ADDR
    {MENUITEM_ACTION,       "Dump OF recovery",     &dump_of_recovery},
#endif
    {MENUITEM_ACTION,       "Dump entire flash",    &dump_entire_flash},
    {MENUITEM_ACTION,       "Main menu",            &to_main_menu},
};

static const struct menuitem* current_menu = NULL;
static struct bl_list recmenu_list;

static void to_main_menu(void)
{
    current_menu = recovery_items;
    recmenu_list.num_items = ARRAYLEN(recovery_items);
    recmenu_list.selected_item = 1;
}

static void to_debug_menu(void)
{
    current_menu = debug_menu_items;
    recmenu_list.num_items = ARRAYLEN(debug_menu_items);
    recmenu_list.selected_item = 1;
}

static void recmenu_draw_item(const struct bl_listitem* item)
{
    const struct menuitem* mu = &current_menu[item->index];
    const char* fmt;

    switch(mu->type) {
    case MENUITEM_HEADING:
        fmt = "[%s]";
        break;

    case MENUITEM_ACTION:
    default:
        if(item->index == item->list->selected_item)
            fmt = "=> %s";
        else
            fmt = "   %s";
        break;
    }

    lcd_putsxyf(item->x, item->y, fmt, mu->text);
}

static void recmenu_scroll(struct bl_list* list, int dir)
{
    int start, end, step;

    if(dir < 0) {
        start = list->selected_item - 1;
        end = -1;
        step = -1;
    } else if(dir > 0) {
        start = list->selected_item + 1;
        end = list->num_items;
        step = 1;
    } else {
        return;
    }

    for(int i = start; i != end; i += step) {
        if(current_menu[i].action) {
            gui_list_select(list, i);

            /* always show one item above the selection to ensure
             * the topmost heading is visible */
            if(list->selected_item == list->top_item && list->top_item > 0)
                list->top_item--;

            break;
        }
    }
}

static void put_help_line(int y, int line, const char* str1, const char* str2)
{
    y += line*SYSFONT_HEIGHT;
    lcd_putsxy(0, y, str1);
    lcd_putsxy(LCD_WIDTH - strlen(str2)*SYSFONT_WIDTH, y, str2);
}

void recovery_menu(void)
{
    struct viewport vp = {
        .x = 0, .y = SYSFONT_HEIGHT,
        .width = LCD_WIDTH,
        .height = LCD_HEIGHT - SYSFONT_HEIGHT*5,
    };
    lcd_init_viewport(&vp);

    struct bl_list* list = &recmenu_list;
    gui_list_init(list, &vp);
    list->draw_item = recmenu_draw_item;
    to_main_menu();

    while(1) {
        clearscreen();
        putcenter_y(0, "Rockbox recovery menu");

        /* draw the help text */
        int ypos = LCD_HEIGHT - 4*SYSFONT_HEIGHT;
        put_help_line(ypos, 0, BL_DOWN_NAME "/" BL_UP_NAME, "move cursor");
        put_help_line(ypos, 1, BL_SELECT_NAME, "select item");
        put_help_line(ypos, 2, BL_QUIT_NAME, "power off");

        /* draw the list */
        gui_list_draw(list);

        lcd_update();

        /* handle input */
        switch(get_button(TIMEOUT_BLOCK)) {
        case BL_SELECT: {
            if(current_menu[list->selected_item].action)
                current_menu[list->selected_item].action();
        } break;

        case BL_UP:
            recmenu_scroll(list, -1);
            break;

        case BL_DOWN:
            recmenu_scroll(list, 1);
            break;

        case BL_QUIT:
            shutdown();
            break;
        }
    }
}
