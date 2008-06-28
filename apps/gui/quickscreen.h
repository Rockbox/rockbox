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

#include "option_select.h"
#include "screen_access.h"

enum QUICKSCREEN_ITEM {
    QUICKSCREEN_LEFT = 0,
    QUICKSCREEN_RIGHT,
    QUICKSCREEN_BOTTOM,
    QUICKSCREEN_ITEM_COUNT,
};

struct gui_quickscreen
{
    const struct settings_list *items[QUICKSCREEN_ITEM_COUNT];
    void (*callback)(struct gui_quickscreen * qs); /* called after a
                                                    item is changed */
};

bool gui_syncquickscreen_run(struct gui_quickscreen * qs, int button_enter);


#ifdef BUTTON_F3
extern bool quick_screen_f3(int button_enter);
#endif
extern bool quick_screen_quick(int button_enter);


#endif /*_GUI_QUICK_SCREEN_H_*/
#endif /* HAVE_QUICKSCREEN */
