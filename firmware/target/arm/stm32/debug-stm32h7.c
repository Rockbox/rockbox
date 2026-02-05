/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 by Aidan MacDonald
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
#include "system.h"
#include "kernel.h"
#include "button.h"
#include "lcd.h"
#include "font.h"
#include "action.h"
#include "list.h"
#include "regs/stm32h743/dbgmcu.h"
#include "regs/cortex-m/cm_debug.h"
#include <stdio.h>

enum
{
    SWD_MENU_IS_ENABLED,
    SWD_MENU_IS_ATTACHED,
    SWD_MENU_NUM_ENTRIES
};

static int swd_menu_action_cb(int action, struct gui_synclist *lists)
{
    int sel = gui_synclist_get_sel_pos(lists);

    if (sel == SWD_MENU_IS_ENABLED && action == ACTION_STD_OK)
    {
        if (reg_readf(DBGMCU_CR, DBGSLEEP_D1))
            system_debug_enable(false);
        else
            system_debug_enable(true);

        action = ACTION_REDRAW;
    }

    if (action == ACTION_NONE)
        action = ACTION_REDRAW;

    return action;
}

static const char *swd_menu_get_name(int item, void *data, char *buf, size_t bufsz)
{
    (void)data;

    switch (item)
    {
    case SWD_MENU_IS_ENABLED:
        snprintf(buf, bufsz, "System debug enabled: %d",
                 (int)!!reg_readf(DBGMCU_CR, DBGSLEEP_D1));
        return buf;

    case SWD_MENU_IS_ATTACHED:
        snprintf(buf, bufsz, "DHCSR.C_DEBUGEN bit: %d",
                 (int)!!reg_readf(CM_DEBUG_DHCSR, C_DEBUGEN));
        return buf;

    default:
        return "";
    }
}

static bool swd_menu(void)
{
    struct simplelist_info info;
    simplelist_info_init(&info, "SWD/JTAG", SWD_MENU_NUM_ENTRIES, NULL);
    info.action_callback = swd_menu_action_cb;
    info.get_name = swd_menu_get_name;
    return simplelist_show_list(&info);
}

/* Menu definition */
static const struct {
    const char *name;
    bool (*function) (void);
} menuitems[] = {
    {"SWD/JTAG", swd_menu},
};

static int hw_info_menu_action_cb(int btn, struct gui_synclist *lists)
{
    if (btn == ACTION_STD_OK)
    {
        int sel = gui_synclist_get_sel_pos(lists);
        FOR_NB_SCREENS(i)
            viewportmanager_theme_enable(i, false, NULL);

        menuitems[sel].function();
        btn = ACTION_REDRAW;

        FOR_NB_SCREENS(i)
            viewportmanager_theme_undo(i, false);
    }

    return btn;
}

static const char* hw_info_menu_get_name(int item, void *data, char *buf, size_t bufsz)
{
    (void)buf;
    (void)bufsz;
    (void)data;
    return menuitems[item].name;
}

bool dbg_hw_info(void)
{
    struct simplelist_info info;
    simplelist_info_init(&info, MODEL_NAME " debug menu",
                         ARRAYLEN(menuitems), NULL);
    info.action_callback = hw_info_menu_action_cb;
    info.get_name = hw_info_menu_get_name;
    return simplelist_show_list(&info);
}

bool dbg_ports(void)
{
    return false;
}
