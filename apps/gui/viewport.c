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



#define LINE_SEL_FROM_SETTINGS(vp) \
    do { \
        vp->lss_pattern = global_settings.lss_color; \
        vp->lse_pattern = global_settings.lse_color; \
        vp->lst_pattern = global_settings.lst_color; \
    } while (0)

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
        LINE_SEL_FROM_SETTINGS(vp);
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

const char* viewport_parse_viewport(struct viewport *vp,
                                    enum screen_type screen,
                                    const char *bufptr,
                                    const char separator)
{
    /* parse the list to the viewport struct */
    const char *ptr = bufptr;
    int depth;
    uint32_t set = 0;

    enum {
        PL_X = 0,
        PL_Y,
        PL_WIDTH,
        PL_HEIGHT,
        PL_FONT,
        PL_FG,
        PL_BG,
    };
    
    /* Work out the depth of this display */
    depth = screens[screen].depth;
#ifdef HAVE_LCD_COLOR
    if (depth == 16)
    {
        if (!(ptr = parse_list("dddddcc", &set, separator, ptr, &vp->x, &vp->y, &vp->width,
                    &vp->height, &vp->font, &vp->fg_pattern,&vp->bg_pattern)))
            return VP_ERROR;
    }
    else 
#endif
#if (LCD_DEPTH == 2) || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH == 2)
    if (depth == 2) {
        if (!(ptr = parse_list("dddddgg", &set, separator, ptr, &vp->x, &vp->y, &vp->width,
                    &vp->height, &vp->font, &vp->fg_pattern, &vp->bg_pattern)))
            return VP_ERROR;
    }
    else 
#endif
#if (LCD_DEPTH == 1) || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH == 1)
    if (depth == 1)
    {
        if (!(ptr = parse_list("ddddd", &set, separator, ptr, &vp->x, &vp->y, &vp->width,
                &vp->height, &vp->font)))
            return VP_ERROR;
    }
    else
#endif
    {}

    /* X and Y *must* be set */
    if (!LIST_VALUE_PARSED(set, PL_X) || !LIST_VALUE_PARSED(set, PL_Y))
        return VP_ERROR;
    
    /* fix defaults */
    if (!LIST_VALUE_PARSED(set, PL_WIDTH))
        vp->width = screens[screen].lcdwidth - vp->x;
    if (!LIST_VALUE_PARSED(set, PL_HEIGHT))
        vp->height = screens[screen].lcdheight - vp->y;

#if (LCD_DEPTH > 1)
    if (!LIST_VALUE_PARSED(set, PL_FG))
        vp->fg_pattern = global_settings.fg_color;
    if (!LIST_VALUE_PARSED(set, PL_BG))
        vp->bg_pattern = global_settings.bg_color;
#endif
#ifdef HAVE_LCD_COLOR
    LINE_SEL_FROM_SETTINGS(vp);
#endif
    /* Validate the viewport dimensions - we know that the numbers are
       non-negative integers, ignore bars and assume the viewport takes them
       * into account */
    if ((vp->x >= screens[screen].lcdwidth) ||
        ((vp->x + vp->width) > screens[screen].lcdwidth) ||
        (vp->y >= screens[screen].lcdheight) ||
        ((vp->y + vp->height) > screens[screen].lcdheight))
    {
        return VP_ERROR;
    }

    /* Default to using the user font if the font was an invalid number or '-'*/
    if (((vp->font != FONT_SYSFIXED) && (vp->font != FONT_UI))
            || !LIST_VALUE_PARSED(set, PL_FONT)
            )
        vp->font = FONT_UI;
        
    vp->drawmode = DRMODE_SOLID;

    return ptr;
}
