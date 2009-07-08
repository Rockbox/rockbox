/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jonathan Gordon
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

#include <stdlib.h>
#include "config.h"
#include "lcd.h"
#include "lcd-remote.h"
#include "font.h"
#include "sprintf.h"
#include "string.h"
#include "settings.h"
#include "kernel.h"
#include "system.h"
#include "misc.h"
#include "viewport.h"
#include "statusbar.h"
#include "screen_access.h"
#include "appevents.h"

static int statusbar_enabled = 0;

int viewport_get_nb_lines(struct viewport *vp)
{
#ifdef HAVE_LCD_BITMAP
    return vp->height/font_get(vp->font)->height;
#else
    (void)vp;
    return 2;
#endif
}

static bool showing_bars(enum screen_type screen)
{
    if (statusbar_enabled & VP_SB_ONSCREEN(screen))
    {
#ifdef HAVE_LCD_BITMAP
        bool ignore = statusbar_enabled & VP_SB_IGNORE_SETTING(screen);
        return ignore || (statusbar_position(screen));
#else
        return true;
#endif
    }
    return false;
}    

void viewport_set_defaults(struct viewport *vp, enum screen_type screen)
{
    vp->x = 0;
    vp->width = screens[screen].lcdwidth;

#ifdef HAVE_LCD_BITMAP
    vp->drawmode = DRMODE_SOLID;
    vp->font = FONT_UI; /* default to UI to discourage SYSFONT use */
        
    vp->height = screens[screen].lcdheight;
    if (statusbar_position(screen) != STATUSBAR_BOTTOM && showing_bars(screen))
        vp->y = STATUSBAR_HEIGHT;
    else 
        vp->y = 0;
#else
    vp->y = 0;
#endif
    vp->height = screens[screen].lcdheight - (showing_bars(screen)?STATUSBAR_HEIGHT:0);

#ifdef HAVE_REMOTE_LCD
    /* We only need this test if there is a remote LCD */
    if (screen == SCREEN_MAIN)
#endif
    {
#ifdef HAVE_LCD_COLOR
        vp->fg_pattern = global_settings.fg_color;
        vp->bg_pattern = global_settings.bg_color;
        vp->lss_pattern = global_settings.lss_color;
        vp->lse_pattern = global_settings.lse_color;
        vp->lst_pattern = global_settings.lst_color;
#elif LCD_DEPTH > 1
        vp->fg_pattern = LCD_DEFAULT_FG;
        vp->bg_pattern = LCD_DEFAULT_BG;
#endif
    }

#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
    if (screen == SCREEN_REMOTE)
    {
        vp->fg_pattern = LCD_REMOTE_DEFAULT_FG;
        vp->bg_pattern = LCD_REMOTE_DEFAULT_BG;
    }
#endif
}


int viewportmanager_set_statusbar(int enabled)
{
    int old = statusbar_enabled;
    statusbar_enabled = enabled;
    if (enabled)
    {
        int i;
        FOR_NB_SCREENS(i)
        {
            if (showing_bars(i))
                gui_statusbar_draw(&statusbars.statusbars[i], true);
        }
        add_event(GUI_EVENT_ACTIONUPDATE, false, viewportmanager_draw_statusbars);
    }
    else
    {
        remove_event(GUI_EVENT_ACTIONUPDATE, viewportmanager_draw_statusbars);
    }
    return old;
}

void viewportmanager_draw_statusbars(void* data)
{
    int i;

    FOR_NB_SCREENS(i)
    {
        if (showing_bars(i))
            gui_statusbar_draw(&statusbars.statusbars[i], (bool)data);
    }
}

void viewportmanager_statusbar_changed(void* data)
{
    (void)data;
    statusbar_enabled = 0;
    if (global_settings.statusbar != STATUSBAR_OFF)
        statusbar_enabled = VP_SB_ONSCREEN(SCREEN_MAIN);
#ifdef HAVE_REMOTE_LCD
    if (global_settings.remote_statusbar != STATUSBAR_OFF)
        statusbar_enabled |= VP_SB_ONSCREEN(SCREEN_REMOTE);
#endif
    viewportmanager_set_statusbar(statusbar_enabled);
}
