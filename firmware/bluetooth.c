/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2015 Lorenzo Miori
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

#include "button.h"
#include "font.h"
#include "action.h"
#include "list.h"
#include "bluetooth.h"
#include "bluetooth_hw.h"
#include "string.h"
#include "stdio.h"

/** Bluetooth HCI debug screen **/

// TBD current BT name, address, visibility, nearby devices etc

/** Bluetooth controller debug screen **/

// TBD, actually will belong to the controller driver itself...

/** Bluetooth Transport debug screen **/

// TBD, serial/usb/spi connection state, speed, tx/rx rate etc

/** Bluetooth HAL debug screen **/

static const char* bluetooth_hw_dgb_entry(int selected_item, void* data,
                                    char* buffer, size_t buffer_len)
{
    (void)data;
    switch(selected_item) {
        case 0:
            snprintf(buffer, buffer_len, "HW Power: %s",
                     bluetooth_hw_is_powered() ? "ON" : "OFF");
            break;
        case 1:
            snprintf(buffer, buffer_len, "HW Reset: %s",
                     bluetooth_hw_is_reset() ? "ON" : "OFF");
            break;
        case 2:
            snprintf(buffer, buffer_len, "HW Awake: %s",
                     bluetooth_hw_is_awake() ? "ON" : "OFF");
            break;
        default:
            break;
    }
    return buffer;
}

static void bluetooth_hw_dbg_action(int item)
{
    switch(item) {
        case 0:
            bluetooth_hw_power(!bluetooth_hw_is_powered());
            break;
        case 1:
            bluetooth_hw_reset(!bluetooth_hw_is_reset());
            break;
        case 2:
            bluetooth_hw_awake(!bluetooth_hw_is_awake());
            break;
         default:
            break;
    }
}

bool bluetooth_hw_dbg_screen(void)
{
    struct gui_synclist lists;
    int action;
    gui_synclist_init(&lists, bluetooth_hw_dgb_entry, NULL, false, 1, NULL);
    gui_synclist_set_title(&lists, "Bluetooth debug", NOICON);
    gui_synclist_set_icon_callback(&lists, NULL);
    gui_synclist_set_color_callback(&lists, NULL);
    gui_synclist_set_nb_items(&lists, 3);

    while(1)
    {
        gui_synclist_draw(&lists);
        list_do_action(CONTEXT_STD, HZ, &lists, &action, LIST_WRAP_UNLESS_HELD);

        if(action == ACTION_STD_CANCEL)
            return false;

        if(action == ACTION_STD_OK) {
            int item = gui_synclist_get_sel_pos(&lists);
            bluetooth_hw_dbg_action(item);
        }

        yield();
    }

    return true;
}
