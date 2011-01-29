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
#include "settings.h"
#include "rbpaths.h"
#include "root_menu.h"
#include "tree.h"


enum {
    APPS,
    GAMES,
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
    return ret;
}

#define ITEM_FLAG (MENU_FUNC_USEPARAM|MENU_FUNC_CHECK_RETVAL)

MENUITEM_FUNCTION(games_item, ITEM_FLAG, ID2P(LANG_PLUGIN_GAMES), 
                  plugins_menu, (void*)GAMES, NULL, Icon_Folder);
MENUITEM_FUNCTION(apps_item,  ITEM_FLAG, ID2P(LANG_PLUGIN_APPS), 
                  plugins_menu, (void*)APPS,  NULL, Icon_Folder);
MENUITEM_FUNCTION(demos_item, ITEM_FLAG, ID2P(LANG_PLUGIN_DEMOS), 
                  plugins_menu, (void*)DEMOS, NULL, Icon_Folder);

MAKE_MENU(plugin_menu, ID2P(LANG_PLUGINS), NULL,
          Icon_Plugin,
          &games_item, &apps_item, &demos_item);
