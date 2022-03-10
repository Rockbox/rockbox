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
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "backlight.h"
#include "backlight-target.h"
#include "font.h"
#include "button.h"
#include "version.h"
#include <string.h>

static bool lcd_inited = false;
extern bool is_usb_connected;

void clearscreen(void)
{
    init_lcd();
    lcd_clear_display();
    putversion();
}

void putversion(void)
{
    int x = (LCD_WIDTH - SYSFONT_WIDTH*strlen(rbversion)) / 2;
    int y = LCD_HEIGHT - SYSFONT_HEIGHT;
    lcd_putsxy(x, y, rbversion);
}

void putcenter_y(int y, const char* msg)
{
    int x = (LCD_WIDTH - SYSFONT_WIDTH*strlen(msg)) / 2;
    lcd_putsxy(x, y, msg);
}

void putcenter_line(int line, const char* msg)
{
    int y = LCD_HEIGHT/2 + (line - 1)*SYSFONT_HEIGHT;
    putcenter_y(y, msg);
}

void splash2(long delay, const char* msg, const char* msg2)
{
    clearscreen();
    putcenter_line(0, msg);
    if(msg2)
        putcenter_line(1, msg2);
    lcd_update();
    sleep(delay);
}

void splash(long delay, const char* msg)
{
    splash2(delay, msg, NULL);
}

int get_button(int timeout)
{
    int btn = button_get_w_tmo(timeout);
    if(btn == SYS_USB_CONNECTED)
        is_usb_connected = true;
    else if(btn == SYS_USB_DISCONNECTED)
        is_usb_connected = false;

    return btn;
}

void init_lcd(void)
{
    if(lcd_inited)
        return;

    lcd_init();
    font_init();
    lcd_setfont(FONT_SYSFIXED);

    /* Clear screen before turning backlight on, otherwise we might
     * display random garbage on the screen */
    lcd_clear_display();
    lcd_update();

    backlight_init();

    lcd_inited = true;
}

void gui_shutdown(void)
{
    if(!lcd_inited)
        return;

    backlight_hw_off();
}

void gui_list_init(struct bl_list* list, struct viewport* vp)
{
    list->vp = vp;
    list->num_items = 0;
    list->selected_item = 0;
    list->top_item = 0;
    list->item_height = SYSFONT_HEIGHT;
    list->draw_item = NULL;
}

void gui_list_draw(struct bl_list* list)
{
    struct bl_listitem item = {
        .list = list,
        .x = 0, .y = 0,
        .width = list->vp->width,
        .height = list->item_height,
    };

    struct viewport* old_vp = lcd_set_viewport(list->vp);
    lcd_clear_viewport();

    int items_on_screen = list->vp->height / list->item_height;
    for(int i = 0; i < items_on_screen; ++i) {
        item.index = list->top_item + i;
        if(item.index >= list->num_items)
            break;

        list->draw_item(&item);

        item.y += item.height;
    }

    lcd_set_viewport(old_vp);
}

void gui_list_select(struct bl_list* list, int item_index)
{
    /* clamp the selection */
    list->selected_item = item_index;

    if(list->selected_item < 0)
        list->selected_item = 0;
    else if(list->selected_item >= list->num_items)
        list->selected_item = list->num_items - 1;

    /* handle scrolling the list view */
    int items_on_screen = list->vp->height / list->item_height;
    int bottom_item = list->top_item + items_on_screen;

    if(list->selected_item < list->top_item) {
        list->top_item = list->selected_item;
    } else if(list->selected_item >= bottom_item) {
        list->top_item = list->selected_item - items_on_screen + 1;
        if(list->top_item < 0)
            list->top_item = 0;
    }
}

void gui_list_scroll(struct bl_list* list, int delta)
{
    gui_list_select(list, list->selected_item + delta);
}
