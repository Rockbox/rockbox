/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

#ifndef __GUI_INGENIC_H__
#define __GUI_INGENIC_H__

/* The target's bootloader header supplies the keymap */
#if defined(HIBY_R1_NATIVE)
# include "x1600/x1600bootloader.h"
#else
# include "x1000/x1000bootloader.h"
#endif
#include "lcd.h"
#include <stdbool.h>
#include <stdint.h>

int  read_btn(void);
void put_help_line(int y, int line, const char* str1, const char* str2);

struct bl_listitem {
    struct bl_list* list;

    int index;
    int x, y, width, height;
};

struct bl_list {
    struct viewport* vp;

    int num_items;
    int selected_item;
    int top_item;
    int item_height;

    void(*draw_item)(const struct bl_listitem* item);
};

void clearscreen(void);
void putversion(void);
void putcenter_y(int y, const char* msg);
void splashf(long delay, const char* msg, ...);
int get_button(int timeout);

void gui_list_init(struct bl_list* list, struct viewport* vp);
void gui_list_draw(struct bl_list* list);
void gui_list_select(struct bl_list* list, int item_index);
void gui_list_scroll(struct bl_list* list, int delta);

/* Implemented by the target. */
void init_lcd(void);
void gui_shutdown(void);

/* Maintained by get_button(). */
extern bool is_usb_connected;
extern intptr_t usb_connection_seqnum;

#endif /* __GUI_INGENIC_H__ */
