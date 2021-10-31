/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Thomas Martitz
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
#include <string.h>
#include "config.h"
#include "lang.h"
#include "menu.h"
#include "action.h"
#include "settings.h"
#include "rbpaths.h"
#include "root_menu.h"
#include "tree.h"

enum {
    GAMES,
    APPS,
    DEMOS,
};

static const struct {
    const char *path;
    int id;
} items[] = {
    { PLUGIN_GAMES_DIR, LANG_PLUGIN_GAMES },
    { PLUGIN_APPS_DIR,  LANG_PLUGIN_APPS  },
    { PLUGIN_DEMOS_DIR, LANG_PLUGIN_DEMOS },
};

/* if handler is active we are waiting to reenter menu */
static void pm_handler(unsigned short id, void *data)
{
    remove_event(id, data);
}

static int plugins_menu(void* param)
{
    intptr_t item = (intptr_t)param;
    struct browse_context browse;
    int ret;

    browse_context_init(&browse, SHOW_PLUGINS, 0, str(items[item].id),
                         Icon_Plugin, items[item].path, NULL);

    ret = rockbox_browse(&browse);

    if (ret == GO_TO_PREVIOUS)
        return 0;
    if (ret == GO_TO_PLUGIN)
        add_event(SYS_EVENT_USB_INSERTED, pm_handler);

    return ret;
}

static int menu_callback(int action,
                         const struct menu_item_ex *this_item,
                         struct gui_synclist *this_list)
{
    (void)this_item;
    static int selected = 0;

    if (action == ACTION_ENTER_MENUITEM)
    {
        this_list->selected_item = selected;
        if (!add_event(SYS_EVENT_USB_INSERTED, pm_handler))
        {
            action = ACTION_STD_OK; /* event exists -- reenter menu */
        }
        remove_event(SYS_EVENT_USB_INSERTED, pm_handler);
    }
    else if (action == ACTION_STD_OK)
    {
        selected = gui_synclist_get_sel_pos(this_list);
    }
    return action;
}

#define ITEM_FLAG (MENU_FUNC_USEPARAM|MENU_FUNC_CHECK_RETVAL)

MENUITEM_FUNCTION(games_item, ITEM_FLAG, ID2P(LANG_PLUGIN_GAMES), 
                  plugins_menu, (void*)GAMES, NULL, Icon_Folder);
MENUITEM_FUNCTION(apps_item,  ITEM_FLAG, ID2P(LANG_PLUGIN_APPS), 
                  plugins_menu, (void*)APPS,  NULL, Icon_Folder);
MENUITEM_FUNCTION(demos_item, ITEM_FLAG, ID2P(LANG_PLUGIN_DEMOS), 
                  plugins_menu, (void*)DEMOS, NULL, Icon_Folder);

MAKE_MENU(plugin_menu, ID2P(LANG_PLUGINS), &menu_callback,
          Icon_Plugin,
          &games_item, &apps_item, &demos_item);
