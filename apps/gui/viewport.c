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

int viewport_get_nb_lines(struct viewport *vp)
{
#ifdef HAVE_LCD_BITMAP
    return vp->height/font_get(vp->font)->height;
#else
    (void)vp;
    return 2;
#endif
}


void viewport_set_defaults(struct viewport *vp, enum screen_type screen)
{
    vp->x = 0;
    vp->width = screens[screen].lcdwidth;

    vp->y = gui_statusbar_height();
    vp->height = screens[screen].lcdheight - vp->y;
#ifdef HAVE_LCD_BITMAP
    vp->drawmode = DRMODE_SOLID;
    vp->font = FONT_UI; /* default to UI to discourage SYSFONT use */
#endif

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
