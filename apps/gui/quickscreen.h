/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
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
#include "config.h"

#ifdef HAVE_QUICKSCREEN

#ifndef _GUI_QUICKSCREEN_H_
#define _GUI_QUICKSCREEN_H_

#include "screen_access.h"

enum quickscreen_item {
    QUICKSCREEN_TOP = 0,
    QUICKSCREEN_LEFT,
    QUICKSCREEN_RIGHT,
    QUICKSCREEN_BOTTOM,
    QUICKSCREEN_ITEM_COUNT,
};

enum quickscreen_return {
    QUICKSCREEN_OK = 0,
    QUICKSCREEN_IN_USB = 0x1,
    QUICKSCREEN_GOTO_SHORTCUTS_MENU = 0x2,
    QUICKSCREEN_CHANGED = 0x4,
};

struct gui_quickscreen
{
    const struct settings_list *items[QUICKSCREEN_ITEM_COUNT];
};

extern int quick_screen_quick(int button_enter);
int quickscreen_set_option(void *data);
bool is_setting_quickscreenable(const struct settings_list *setting);

#endif /*_GUI_QUICK_SCREEN_H_*/
#endif /* HAVE_QUICKSCREEN */
